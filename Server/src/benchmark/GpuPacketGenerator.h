#ifndef GPU_PACKET_GENERATOR_H
#define GPU_PACKET_GENERATOR_H

#include "IPacketGenerator.h"
#include "IcmpFuzzStrategy.h"
#include <cuda_runtime.h>

namespace fuzzer {

class GpuPacketGenerator : public IPacketGenerator {
public:
    GpuPacketGenerator(const PacketGenConfig& config);
    ~GpuPacketGenerator();
    
    void generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) override;
    std::string get_name() const override { return "GPU (CUDA)"; }
    int get_thread_count() const override { return 0; } // CUDA managed
    bool is_gpu() const override { return true; }

private:
    IcmpFuzzStrategy strategy_;
    cudaStream_t stream_;
    uint8_t* h_output_;
    uint16_t* h_lengths_;
    uint8_t* d_output_;
    uint16_t* d_lengths_;
    size_t allocated_size_ = 0;
};

} // namespace fuzzer

#endif // GPU_PACKET_GENERATOR_H
