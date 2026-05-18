#include "MultiThreadGenerator.h"
#include "CpuUtils.h"
#include <vector>

namespace fuzzer {

MultiThreadGenerator::MultiThreadGenerator(int thread_count) {
    if (thread_count <= 0) {
        thread_count_ = std::thread::hardware_concurrency();
    } else {
        thread_count_ = thread_count;
    }
}

void MultiThreadGenerator::generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) {
    std::vector<std::thread> threads;
    uint32_t packets_per_thread = batch_size / thread_count_;

    for (int t = 0; t < thread_count_; ++t) {
        uint32_t start = t * packets_per_thread;
        uint32_t end = (t == thread_count_ - 1) ? batch_size : (t + 1) * packets_per_thread;

        threads.emplace_back([this, output, lengths, start, end, &config]() {
            for (uint32_t i = start; i < end; ++i) {
                uint8_t* pkt = output + (static_cast<size_t>(i) * 1514);
                generate_icmp_packet(pkt, i, config.src_mac, config.dest_mac, config.src_ip, config.dest_ip);
                lengths[i] = 14 + sizeof(IpV4Hdr) + sizeof(IcmpHdr);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

} // namespace fuzzer
