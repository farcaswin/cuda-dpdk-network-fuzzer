#ifndef TRANSMISSION_BENCHMARK_H
#define TRANSMISSION_BENCHMARK_H

#include <vector>
#include <string>
#include <cstdint>
#include "IPacketGenerator.h"
#include "LinuxSocketSender.h"
#include "DpdkSender.h"
#include "FuzzEngine.h"
#include "NotificationHub.h"

namespace fuzzer {

struct PipelineRun {
    std::string pipeline_name;
    uint32_t batch_size;
    double total_pps;
    double duration_ms;
    uint64_t total_sent;
};

struct PipelineReport {
    std::vector<PipelineRun> runs;
    std::string to_table() const;
    std::string to_csv() const;
};

class TransmissionBenchmark {
public:
    TransmissionBenchmark(DpdkSender& dpdk_sender, const std::string& linux_iface);
    
    PipelineRun run_linux_sockets(IPacketGenerator& gen, uint32_t batch_size, const PacketGenConfig& config, int iterations = 100);
    PipelineRun run_dpdk(IPacketGenerator& gen, uint32_t batch_size, const PacketGenConfig& config, int iterations = 100);
    
    // New methods using the real FuzzEngine
    PipelineRun run_fuzz_engine(uint32_t batch_size, const PacketGenConfig& config, int duration_sec = 2);
    PipelineRun run_fuzz_engine_icmp(uint32_t batch_size, const PacketGenConfig& config, int duration_sec = 2);

    PipelineReport run_all(const PacketGenConfig& config);

private:
    DpdkSender& dpdk_sender_;
    std::string linux_iface_;
    NotificationHub hub_;
    std::unique_ptr<FuzzEngine> engine_; // Persistent engine for reuse
};

} // namespace fuzzer

#endif // TRANSMISSION_BENCHMARK_H
