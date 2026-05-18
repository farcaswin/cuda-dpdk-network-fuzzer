#include "TransmissionBenchmark.h"
#include "SingleThreadGenerator.h"
#include "MultiThreadGenerator.h"
#include "GpuPacketGenerator.h"
#include "HeavyPayloadGenerator.h"
#include "IcmpFuzzStrategy.h"
#include "HeavyPayloadFuzzStrategy.h"
#include "Logger.h"
#include <chrono>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <thread>

namespace fuzzer {

using Clock = std::chrono::high_resolution_clock;

TransmissionBenchmark::TransmissionBenchmark(DpdkSender& dpdk_sender, const std::string& linux_iface)
    : dpdk_sender_(dpdk_sender), linux_iface_(linux_iface) {}

PipelineRun TransmissionBenchmark::run_linux_sockets(IPacketGenerator& gen, uint32_t batch_size, const PacketGenConfig& config, int iterations) {
    PipelineRun run;
    run.pipeline_name = gen.get_name() + " + Linux Sockets";
    run.batch_size = batch_size;
    run.total_sent = 0;
    run.duration_ms = 0;
    run.total_pps = 0.0;

    try {
        LinuxSocketSender sender(linux_iface_);
        std::vector<uint8_t> buffer(batch_size * 1514);
        std::vector<uint16_t> lengths(batch_size);

        LOG_INFO("[Pipeline] Testing Linux Sockets with {}", gen.get_name());

        auto t0 = Clock::now();
        uint64_t total_sent = 0;

        for (int i = 0; i < iterations; ++i) {
            gen.generate(buffer.data(), lengths.data(), batch_size, config);
            total_sent += sender.send_burst(buffer.data(), lengths.data(), batch_size);
        }

        auto t1 = Clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        run.total_sent = total_sent;
        run.duration_ms = ms;
        run.total_pps = (total_sent / ms) * 1000.0;

    } catch (const std::exception& e) {
        LOG_WARN("[Pipeline] Skipping Linux Sockets for {}: {}", gen.get_name(), e.what());
    }

    return run;
}

PipelineRun TransmissionBenchmark::run_dpdk(IPacketGenerator& gen, uint32_t batch_size, const PacketGenConfig& config, int iterations) {
    std::vector<uint8_t> buffer(batch_size * 1514);
    std::vector<uint16_t> lengths(batch_size);

    LOG_INFO("[Pipeline] Testing DPDK with {}", gen.get_name());

    auto t0 = Clock::now();
    uint64_t total_sent = 0;

    for (int i = 0; i < iterations; ++i) {
        gen.generate(buffer.data(), lengths.data(), batch_size, config);
        total_sent += dpdk_sender_.send_burst(buffer.data(), lengths.data(), batch_size);
    }

    auto t1 = Clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    PipelineRun run;
    run.pipeline_name = gen.get_name() + " + DPDK";
    run.batch_size = batch_size;
    run.total_sent = total_sent;
    run.duration_ms = ms;
    run.total_pps = (total_sent / ms) * 1000.0;

    return run;
}

PipelineRun TransmissionBenchmark::run_fuzz_engine(uint32_t batch_size, const PacketGenConfig& config, int duration_sec) {
    LOG_INFO("[Pipeline] Testing GPU HEAVY (Production Engine) | batch={}", batch_size);
    
    if (!engine_) engine_ = std::make_unique<FuzzEngine>(dpdk_sender_, hub_);
    
    HeavyPayloadFuzzStrategy::Config strat_config;
    std::copy(config.dest_mac, config.dest_mac + 6, strat_config.dest_mac);
    std::copy(config.src_mac, config.src_mac + 6, strat_config.src_mac);
    strat_config.dest_ip = config.dest_ip;
    strat_config.src_ip = config.src_ip;
    
    engine_->start(std::make_unique<HeavyPayloadFuzzStrategy>(strat_config), batch_size, 0, duration_sec);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_sec * 1000 + 500));
    
    PipelineRun run;
    run.pipeline_name = "GPU (Heavy Payload) + DPDK";
    run.batch_size = batch_size;
    run.total_sent = engine_->get_stats().packets_sent.load();
    run.total_pps = engine_->get_stats().current_pps.load();
    run.duration_ms = duration_sec * 1000.0;
    
    engine_->stop();
    return run;
}

PipelineRun TransmissionBenchmark::run_fuzz_engine_icmp(uint32_t batch_size, const PacketGenConfig& config, int duration_sec) {
    LOG_INFO("[Pipeline] Testing GPU ICMP (Production Engine) | batch={}", batch_size);
    
    if (!engine_) engine_ = std::make_unique<FuzzEngine>(dpdk_sender_, hub_);
    
    IcmpFuzzStrategy::Config strat_config;
    std::copy(config.dest_mac, config.dest_mac + 6, strat_config.dest_mac);
    std::copy(config.src_mac, config.src_mac + 6, strat_config.src_mac);
    strat_config.dest_ip = config.dest_ip;
    strat_config.src_ip = config.src_ip;
    
    engine_->start(std::make_unique<IcmpFuzzStrategy>(strat_config), batch_size, 0, duration_sec);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_sec * 1000 + 500));
    
    PipelineRun run;
    run.pipeline_name = "GPU (ICMP) + DPDK";
    run.batch_size = batch_size;
    run.total_sent = engine_->get_stats().packets_sent.load();
    run.total_pps = engine_->get_stats().current_pps.load();
    run.duration_ms = duration_sec * 1000.0;
    
    engine_->stop();
    return run;
}

PipelineReport TransmissionBenchmark::run_all(const PacketGenConfig& config) {
    PipelineReport report;
    // Standard test batches
    std::vector<uint32_t> test_batches = {1024, 4096, 16384, 65536};
    
    SingleThreadGenerator st_gen;
    MultiThreadGenerator mt_gen;
    HeavyPayloadGenerator heavy_cpu_gen;

    for (uint32_t batch : test_batches) {
        // 1. CPU ST + Linux Sockets
        report.runs.push_back(run_linux_sockets(st_gen, batch, config));
        
        // 2. CPU ST + DPDK
        report.runs.push_back(run_dpdk(st_gen, batch, config));
        
        // 3. CPU MT + DPDK
        report.runs.push_back(run_dpdk(mt_gen, batch, config));

        // 4. CPU Heavy + DPDK
        report.runs.push_back(run_dpdk(heavy_cpu_gen, batch, config));
        
        // 5. GPU (Production Engine) + DPDK
        report.runs.push_back(run_fuzz_engine(batch, config, 2));

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    }
    
    return report;
}

std::string PipelineReport::to_table() const {
    std::ostringstream oss;
    oss << "\n";
    oss << "┌──────────────────────────────────────────┬──────────┬──────────────┬────────────┐\n";
    oss << "│ Pipeline                                 │ Batch    │ Total Sent   │ PPS        │\n";
    oss << "├──────────────────────────────────────────┼──────────┼──────────────┼────────────┤\n";

    for (const auto& r : runs) {
        oss << "│ " << std::setw(40) << std::left << r.pipeline_name
            << " │ " << std::setw(8) << std::right << r.batch_size
            << " │ " << std::setw(12) << r.total_sent
            << " │ " << std::setw(10) << std::fixed << std::setprecision(0) << r.total_pps << " │\n";
    }

    oss << "└──────────────────────────────────────────┴──────────┴──────────────┴────────────┘\n";
    return oss.str();
}

std::string PipelineReport::to_csv() const {
    std::ostringstream oss;
    oss << "pipeline,batch_size,total_sent,pps\n";
    for (const auto& r : runs) {
        oss << r.pipeline_name << "," << r.batch_size << "," << r.total_sent << ","
            << std::fixed << std::setprecision(2) << r.total_pps << "\n";
    }
    return oss.str();
}

} // namespace fuzzer
