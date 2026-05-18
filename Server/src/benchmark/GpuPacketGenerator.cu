#include "GpuPacketGenerator.h"
#include <cstring>

namespace fuzzer {

GpuPacketGenerator::GpuPacketGenerator(const PacketGenConfig& config) 
    : strategy_({
        {config.src_mac[0], config.src_mac[1], config.src_mac[2], config.src_mac[3], config.src_mac[4], config.src_mac[5]},
        {config.dest_mac[0], config.dest_mac[1], config.dest_mac[2], config.dest_mac[3], config.dest_mac[4], config.dest_mac[5]},
        config.src_ip,
        config.dest_ip
    }) {
    cudaStreamCreate(&stream_);
    // Ensure we can use mapped memory
    cudaSetDeviceFlags(cudaDeviceMapHost);
    
    // Pre-allocate a large buffer on GPU for benchmarking (up to 256K packets)
    allocated_size_ = 256 * 1024;

    // Use MAPPED pinned memory to match the new architecture
    cudaHostAlloc(&h_output_, allocated_size_ * 1514, cudaHostAllocMapped);
    cudaHostAlloc(&h_lengths_, allocated_size_ * sizeof(uint16_t), cudaHostAllocMapped);

    cudaHostGetDevicePointer(&d_output_, h_output_, 0);
    cudaHostGetDevicePointer(&d_lengths_, h_lengths_, 0);
}

GpuPacketGenerator::~GpuPacketGenerator() {
    cudaStreamDestroy(stream_);
    cudaFreeHost(h_output_);
    cudaFreeHost(h_lengths_);
}

void GpuPacketGenerator::generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) {
    if (batch_size > allocated_size_) batch_size = allocated_size_;
    
    // 1. Launch kernel (Direct Write to Host RAM via PCIe)
    strategy_.launch_kernel(d_output_, d_lengths_, batch_size, stream_);
    
    // 2. Synchronize to ensure writes are finished
    cudaStreamSynchronize(stream_);

    // 3. Since the benchmark expects data in the passed 'output' and 'lengths' buffers,
    // we do a final copy. Note: In PRODUCTION, this step doesn't exist because DPDK 
    // reads directly from the h_output_ buffer.
    std::memcpy(output, h_output_, (size_t)batch_size * 1514);
    std::memcpy(lengths, h_lengths_, (size_t)batch_size * sizeof(uint16_t));
}

} // namespace fuzzer
