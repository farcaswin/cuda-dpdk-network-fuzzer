#include "FuzzService.h"
#include "IcmpFuzzStrategy.h"
#include "IpHeaderFuzzStrategy.h"
#include "TcpFuzzStrategy.h"
#include "Logger.h"
#include <arpa/inet.h>
#include <cstdio>
#include <stdexcept>

namespace fuzzer {

FuzzService::FuzzService(DpdkSender& dpdk_sender) 
    : dpdk_sender_(dpdk_sender), engine_(dpdk_sender) {}

void FuzzService::start_fuzzing(const FuzzParams& params) {
    if (engine_.is_running()) {
        throw std::runtime_error("Fuzzing is already running");
    }

    std::unique_ptr<IFuzzStrategy> strategy;
    
    // Setup common config
    uint32_t dest_ip = ip_to_uint(params.target_ip);
    uint32_t src_ip = ip_to_uint(params.src_ip);
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    mac_to_array(params.target_mac, dest_mac);
    mac_to_array(params.src_mac, src_mac);

    if (params.strategy == "ICMP_TYPES") {
        IcmpFuzzStrategy::Config cfg;
        memcpy(cfg.dest_mac, dest_mac, 6);
        memcpy(cfg.src_mac, src_mac, 6);
        cfg.dest_ip = dest_ip;
        cfg.src_ip = src_ip;
        strategy = std::make_unique<IcmpFuzzStrategy>(cfg);
    } else if (params.strategy == "IP_HEADER") {
        IpHeaderFuzzStrategy::Config cfg;
        memcpy(cfg.dest_mac, dest_mac, 6);
        memcpy(cfg.src_mac, src_mac, 6);
        cfg.dest_ip = dest_ip;
        cfg.src_ip = src_ip;
        strategy = std::make_unique<IpHeaderFuzzStrategy>(cfg);
    } else if (params.strategy == "TCP_FLAGS") {
        TcpFuzzStrategy::Config cfg;
        memcpy(cfg.dest_mac, dest_mac, 6);
        memcpy(cfg.src_mac, src_mac, 6);
        cfg.dest_ip = dest_ip;
        cfg.src_ip = src_ip;
        cfg.dest_port_base = 0;
        strategy = std::make_unique<TcpFuzzStrategy>(cfg);
    } else {
        throw std::runtime_error("Unknown strategy: " + params.strategy);
    }

    engine_.start(std::move(strategy), params.batch_size);
}

void FuzzService::stop_fuzzing() {
    engine_.stop();
}

bool FuzzService::is_fuzzing() const {
    return engine_.is_running();
}

nlohmann::json FuzzService::get_status() const {
    auto& stats = const_cast<FuzzEngine&>(engine_).get_stats();
    return {
        {"running", engine_.is_running()},
        {"packets_sent", stats.packets_sent.load()},
        {"current_pps", stats.current_pps.load()},
        {"target_alive", stats.target_alive.load()}
    };
}

uint32_t FuzzService::ip_to_uint(const std::string& ip) const {
    struct in_addr addr;
    if (inet_aton(ip.c_str(), &addr) == 0) {
        throw std::runtime_error("Invalid IP address: " + ip);
    }
    return addr.s_addr; // already in network byte order
}

void FuzzService::mac_to_array(const std::string& mac, uint8_t* out) const {
    if (sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
               &out[0], &out[1], &out[2], &out[3], &out[4], &out[5]) != 6) {
        throw std::runtime_error("Invalid MAC address: " + mac);
    }
}

} // namespace fuzzer
