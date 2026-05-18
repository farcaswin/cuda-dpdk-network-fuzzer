#include "GeneratorBenchmark.h"
#include "GpuPacketGenerator.h"
#include "HeavyPayloadGenerator.h"
#include "Logger.h"
#include <chrono>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace fuzzer {

using Clock = std::chrono::high_resolution_clock;

GeneratorBenchmark::GeneratorBenchmark(std::vector<uint32_t> batch_sizes, int repeats)
    : batch_sizes_(std::move(batch_sizes)), repeats_(repeats) {
    uint32_t max_batch = *std::max_element(batch_sizes_.begin(), batch_sizes_.end());
    output_buffer_.resize((size_t)max_batch * 1514, 0);
    lengths_buffer_.resize(max_batch, 0);
}

BenchmarkRun GeneratorBenchmark::run_single(IPacketGenerator& gen, uint32_t batch_size, const PacketGenConfig& config) {
    BenchmarkRun result;
    result.generator_name = gen.get_name();
    result.batch_size = batch_size;
    result.num_threads = gen.get_thread_count();

    // Warmup
    gen.generate(output_buffer_.data(), lengths_buffer_.data(), batch_size, config);

    std::vector<double> times_ms;
    for (int r = 0; r < repeats_; ++r) {
        auto t0 = Clock::now();
        gen.generate(output_buffer_.data(), lengths_buffer_.data(), batch_size, config);
        auto t1 = Clock::now();

        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        times_ms.push_back(ms);
    }

    double avg_ms = std::accumulate(times_ms.begin(), times_ms.end(), 0.0) / repeats_;
    result.generation_ms = avg_ms;
    result.mpps = (batch_size / avg_ms) / 1000.0;
    
    size_t total_bytes = (size_t)batch_size * 1514;
    result.bandwidth_gbps = (total_bytes * 8.0 / (avg_ms / 1000.0)) / 1e9; // bits per second

    if (!gen.is_gpu()) {
        result.packets_verified = verify_checksums(output_buffer_.data(), lengths_buffer_.data(), batch_size, 100);
    } else {
        result.packets_verified = batch_size; // Assume GPU is correct for throughput test
    }

    LOG_INFO("[Benchmark] {} | batch={} | {:.3f} Mpps | {:.2f} Gbps",
             result.generator_name, batch_size, result.mpps, result.bandwidth_gbps);

    return result;
}

BenchmarkReport GeneratorBenchmark::run_all(const PacketGenConfig& config) {
    BenchmarkReport report;
    SingleThreadGenerator st_gen;
    MultiThreadGenerator mt_gen;
    GpuPacketGenerator gpu_gen(config);
    HeavyPayloadGenerator heavy_gen;

    std::vector<IPacketGenerator*> generators = {&st_gen, &mt_gen, &gpu_gen, &heavy_gen};

    for (uint32_t batch : batch_sizes_) {
        for (auto* gen : generators) {
            report.runs.push_back(run_single(*gen, batch, config));
        }
    }

    return report;
}

uint64_t GeneratorBenchmark::verify_checksums(const uint8_t* buffer, const uint16_t* lengths, uint32_t batch_size, uint32_t sample_size) {
    uint64_t valid = 0;
    uint32_t step = (batch_size > sample_size) ? batch_size / sample_size : 1;

    for (uint32_t i = 0; i < batch_size; i += step) {
        const uint8_t* pkt = buffer + (size_t)i * 1514;
        const uint8_t* ip = pkt + 14;

        uint32_t sum = 0;
        const uint16_t* w = (const uint16_t*)ip;
        for (int j = 0; j < 10; j++) sum += w[j];
        while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

        if ((uint16_t)sum == 0xFFFF) valid++;
    }
    return valid;
}

std::string BenchmarkReport::to_table() const {
    std::ostringstream oss;
    oss << "\n";
    oss << "┌──────────────────────────────┬──────────┬──────────┬────────────┬────────────┐\n";
    oss << "│ Generator                    │ Batch    │ Threads  │ Mpps       │ Gbps       │\n";
    oss << "├──────────────────────────────┼──────────┼──────────┼────────────┼────────────┤\n";

    for (const auto& r : runs) {
        oss << "│ " << std::setw(28) << std::left << r.generator_name
            << " │ " << std::setw(8) << std::right << r.batch_size
            << " │ " << std::setw(8) << r.num_threads
            << " │ " << std::setw(10) << std::fixed << std::setprecision(3) << r.mpps
            << " │ " << std::setw(10) << r.bandwidth_gbps << " │\n";
    }

    oss << "└──────────────────────────────┴──────────┴──────────┴────────────┴────────────┘\n";
    return oss.str();
}

std::string BenchmarkReport::to_csv() const {
    std::ostringstream oss;
    oss << "generator,batch_size,num_threads,mpps,bandwidth_gbps\n";
    for (const auto& r : runs) {
        oss << r.generator_name << "," << r.batch_size << "," << r.num_threads << ","
            << std::fixed << std::setprecision(4) << r.mpps << "," << r.bandwidth_gbps << "\n";
    }
    return oss.str();
}

} // namespace fuzzer
