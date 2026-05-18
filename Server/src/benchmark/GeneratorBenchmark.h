#ifndef GENERATOR_BENCHMARK_H
#define GENERATOR_BENCHMARK_H

#include <vector>
#include <string>
#include <cstdint>
#include "IPacketGenerator.h"
#include "SingleThreadGenerator.h"
#include "MultiThreadGenerator.h"

namespace fuzzer {

struct BenchmarkRun {
    std::string generator_name;
    uint32_t batch_size;
    int num_threads;
    double generation_ms;
    double mpps;
    double bandwidth_gbps;
    uint64_t packets_verified;
};

struct BenchmarkReport {
    std::vector<BenchmarkRun> runs;
    std::string to_table() const;
    std::string to_csv() const;
};

class GeneratorBenchmark {
public:
    GeneratorBenchmark(std::vector<uint32_t> batch_sizes, int repeats = 5);
    
    BenchmarkRun run_single(IPacketGenerator& gen, uint32_t batch_size, const PacketGenConfig& config);
    BenchmarkReport run_all(const PacketGenConfig& config);

private:
    uint64_t verify_checksums(const uint8_t* buffer, const uint16_t* lengths, uint32_t batch_size, uint32_t sample_size);
    
    std::vector<uint32_t> batch_sizes_;
    int repeats_;
    std::vector<uint8_t> output_buffer_;
    std::vector<uint16_t> lengths_buffer_;
};

} // namespace fuzzer

#endif // GENERATOR_BENCHMARK_H
