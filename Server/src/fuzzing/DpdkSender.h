#ifndef DPDK_SENDER_H
#define DPDK_SENDER_H

#include <string>
#include <vector>
#include <cstdint>

// Forward declarations to avoid including DPDK headers in the header
struct rte_mempool;

namespace fuzzer {

class DpdkSender {
public:
    DpdkSender();
    ~DpdkSender();

    /**
     * @brief Initialize DPDK EAL, Mempool and Port.
     * @param argc Argument count for EAL init
     * @param argv Arguments for EAL init
     * @return true if initialization was successful
     */
    bool init(int argc, char** argv);

    /**
     * @brief Shutdown DPDK and release resources.
     */
    void shutdown();

    /**
     * @brief Check if DPDK is initialized.
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Sends a large batch of packets by splitting it into micro-bursts.
     * @param packet_data Pointer to the start of the pinned memory buffer
     * @param packet_lengths Pointer to the array of packet lengths
     * @param total_packets Total number of packets in the large batch
     * @return Total number of packets successfully sent
     */
    uint32_t send_burst(const uint8_t* packet_data, const uint16_t* packet_lengths, uint32_t total_packets);

private:
    /**
     * @brief Verify and allocate HugePages if necessary.
     * @param num_pages Number of 2MB pages to allocate
     * @return true if pages are available or successfully allocated
     */
    bool allocate_hugepages(int num_pages);

    /**
     * @brief Initialize the mbuf mempool.
     */
    bool init_mempool();

    /**
     * @brief Initialize the DPDK port (TAP or physical NIC).
     */
    bool init_port();

    /**
     * @brief Connect a TAP interface to a Linux bridge.
     */
    void setup_bridge_link(const std::string& tap_iface, const std::string& bridge_name);

    struct rte_mempool* mempool_ = nullptr;
    uint16_t port_id_ = 0;
    bool initialized_ = false;

    static constexpr uint32_t MAX_PACKET_SIZE = 1514;
    static constexpr uint16_t DPDK_BURST_SIZE = 64;
    static constexpr uint32_t NUM_MBUFS = 153599; // Total mbufs
    static constexpr uint32_t MBUF_CACHE_SIZE = 250;
    static constexpr uint16_t RX_RING_SIZE = 1024;
    static constexpr uint16_t TX_RING_SIZE = 1024;
};

} // namespace fuzzer

#endif // DPDK_SENDER_H
