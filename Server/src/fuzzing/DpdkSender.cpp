#include "DpdkSender.h"
#include "Logger.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_errno.h>

namespace fuzzer {

DpdkSender::DpdkSender() {}

DpdkSender::~DpdkSender() {
    shutdown();
}

bool DpdkSender::init(int argc, char** argv) {
    if (initialized_) return true;

    LOG_INFO("Initializing DPDK Sender...");

    // Step 1: Ensure HugePages
    // 512 pages * 2MB = 1024MB (1GB)
    if (!allocate_hugepages(512)) {
        LOG_ERROR("Failed to allocate HugePages. Performance will be degraded or EAL will fail.");
    }

    // Step 2: Initialize EAL
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        LOG_ERROR("Error with EAL initialization: {}", rte_strerror(rte_errno));
        return false;
    }

    // Step 3: Initialize Mempool
    if (!init_mempool()) {
        LOG_ERROR("Failed to initialize DPDK mempool");
        return false;
    }

    // Step 4: Initialize Port
    if (!init_port()) {
        LOG_ERROR("Failed to initialize DPDK port");
        return false;
    }

    initialized_ = true;
    
    // Step 5: Connect to bridge (Virtual Environment Setup)
    setup_bridge_link("dpdk0", "virbr1");

    LOG_INFO("DPDK Sender initialized successfully");
    return true;
}

void DpdkSender::shutdown() {
    if (initialized_) {
        LOG_INFO("Shutting down DPDK Sender...");
        
        // Bring link down explicitly
        system("ip link set dpdk0 down > /dev/null 2>&1");

        // Close port
        rte_eth_dev_stop(port_id_);
        rte_eth_dev_close(port_id_);
        
        // EAL Cleanup
        rte_eal_cleanup();
        initialized_ = false;
    }
}

bool DpdkSender::allocate_hugepages(int num_pages) {
    const std::string path = "/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages";
    
    // Check current number of hugepages
    std::ifstream ifs(path);
    int current_pages = 0;
    if (ifs.is_open()) {
        ifs >> current_pages;
        ifs.close();
    }

    if (current_pages >= num_pages) {
        LOG_INFO("HugePages already allocated: {}", current_pages);
        return true;
    }

    LOG_INFO("Allocating {} HugePages (2MB each)...", num_pages);
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        LOG_ERROR("Could not open {} for writing. Are you root?", path);
        return false;
    }

    ofs << num_pages;
    ofs.close();

    // Verify allocation
    std::ifstream ifs_verify(path);
    int verified_pages = 0;
    if (ifs_verify.is_open()) {
        ifs_verify >> verified_pages;
        ifs_verify.close();
    }

    if (verified_pages < num_pages) {
        LOG_WARN("HugePages allocation partial: requested {}, got {}", num_pages, verified_pages);
        return verified_pages > 0;
    }

    return true;
}

bool DpdkSender::init_mempool() {
    // Create mempool for mbufs
    mempool_ = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mempool_ == nullptr) {
        LOG_ERROR("Cannot create mempool: {}", rte_strerror(rte_errno));
        return false;
    }
    LOG_INFO("Mbuf mempool created: {} mbufs", NUM_MBUFS);
    return true;
}

bool DpdkSender::init_port() {
    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0) {
        LOG_ERROR("No DPDK ports available. Ensure --vdev is passed if using TAP.");
        return false;
    }

    // Use the first available port
    port_id_ = 0;
    
    struct rte_eth_conf port_conf = {};
    // Basic configuration
    port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

    struct rte_eth_dev_info dev_info;
    int ret = rte_eth_dev_info_get(port_id_, &dev_info);
    if (ret != 0) {
        LOG_ERROR("Error during getting device (port {}) info: {}", port_id_, rte_strerror(-ret));
        return false;
    }

    // Configure port with 1 RX and 1 TX queue
    ret = rte_eth_dev_configure(port_id_, 1, 1, &port_conf);
    if (ret < 0) {
        LOG_ERROR("Cannot configure device: port={}, err={}", port_id_, ret);
        return false;
    }

    // Setup RX queue
    ret = rte_eth_rx_queue_setup(port_id_, 0, RX_RING_SIZE,
        rte_eth_dev_socket_id(port_id_), NULL, mempool_);
    if (ret < 0) {
        LOG_ERROR("RX queue setup failed: port={}, err={}", port_id_, ret);
        return false;
    }

    // Setup TX queue
    ret = rte_eth_tx_queue_setup(port_id_, 0, TX_RING_SIZE,
        rte_eth_dev_socket_id(port_id_), NULL);
    if (ret < 0) {
        LOG_ERROR("TX queue setup failed: port={}, err={}", port_id_, ret);
        return false;
    }

    // Start the Ethernet port
    ret = rte_eth_dev_start(port_id_);
    if (ret < 0) {
        LOG_ERROR("rte_eth_dev_start: err={}, port={}", ret, port_id_);
        return false;
    }

    // Enable promiscuous mode
    ret = rte_eth_promiscuous_enable(port_id_);
    if (ret != 0) {
        LOG_WARN("Could not enable promiscuous mode on port {}: {}", port_id_, rte_strerror(-ret));
    }

    LOG_INFO("DPDK Port {} started", port_id_);
    return true;
}

void DpdkSender::setup_bridge_link(const std::string& tap_iface, const std::string& bridge_name) {
    // 1. Check if bridge exists
    std::string check_bridge = "ip link show " + bridge_name + " > /dev/null 2>&1";
    if (system(check_bridge.c_str()) != 0) {
        LOG_WARN("Bridge {} not found. TAP interface {} will not be bridged.", bridge_name, tap_iface);
        return;
    }

    LOG_INFO("Connecting {} to {}...", tap_iface, bridge_name);
    
    // 2. Set master and bring up
    // We use a small delay or retry to ensure the TAP is visible to the kernel after DPDK starts
    std::string cmd = "ip link set " + tap_iface + " master " + bridge_name + " && ip link set " + tap_iface + " up";
    
    int ret = system(cmd.c_str());
    if (ret == 0) {
        LOG_INFO("Successfully bridged {} to {}", tap_iface, bridge_name);
    } else {
        LOG_ERROR("Failed to bridge {} to {}. Manual command might be needed: sudo {}", tap_iface, bridge_name, cmd);
    }
}

uint32_t DpdkSender::send_burst(const uint8_t* packet_data, const uint16_t* packet_lengths, uint32_t total_packets) {
    if (!initialized_) return 0;

    uint32_t total_sent = 0;
    struct rte_mbuf* mbufs[DPDK_BURST_SIZE];

    for (uint32_t i = 0; i < total_packets; i += DPDK_BURST_SIZE) {
        uint16_t current_burst_size = std::min((uint32_t)DPDK_BURST_SIZE, total_packets - i);

        // 1. Bulk allocation of mbufs
        if (rte_pktmbuf_alloc_bulk(mempool_, mbufs, current_burst_size) != 0) {
            LOG_WARN("Mempool exhaustion! Failed to allocate {} mbufs.", current_burst_size);
            break;
        }

        // 2. Prepare each mbuf in the burst
        for (uint16_t j = 0; j < current_burst_size; ++j) {
            uint32_t global_idx = i + j;
            uint16_t len = packet_lengths[global_idx];
            
            // Safety check for length
            if (len > MAX_PACKET_SIZE || len == 0) {
                len = MAX_PACKET_SIZE; 
            }

            // Calculate offset in the giant pinned memory buffer
            const uint8_t* src = packet_data + (static_cast<size_t>(global_idx) * MAX_PACKET_SIZE);

            // Copy data to mbuf
            char* dst = rte_pktmbuf_append(mbufs[j], len);
            if (dst) {
                rte_memcpy(dst, src, len);
            } else {
                LOG_ERROR("Failed to append {} bytes to mbuf", len);
            }
        }

        // 3. Transmit the burst
        uint16_t sent = rte_eth_tx_burst(port_id_, 0, mbufs, current_burst_size);
        total_sent += sent;

        // 4. Free mbufs that were not sent (TX ring full)
        if (sent < current_burst_size) {
            for (uint16_t k = sent; k < current_burst_size; ++k) {
                rte_pktmbuf_free(mbufs[k]);
            }
        }
    }

    return total_sent;
}

} // namespace fuzzer
