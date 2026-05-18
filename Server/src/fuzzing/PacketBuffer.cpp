#include "PacketBuffer.h"
#include "Logger.h"
#include <stdexcept>

namespace fuzzer {

PacketBuffer::PacketBuffer(uint32_t max_batch_size, uint32_t max_packet_size)
    : capacity_(max_batch_size), current_batch_size_(max_batch_size), max_packet_size_(max_packet_size) {

    LOG_INFO("Allocating Zero-Copy PacketBuffer: capacity={}, total_memory={} MB (Pinned Host)", 
             capacity_, 
             (2 * capacity_ * max_packet_size_) / (1024 * 1024));

    // Ensure we can use mapped memory
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    if (!prop.canMapHostMemory) {
        throw std::runtime_error("Device does not support mapping host memory!");
    }
    cudaSetDeviceFlags(cudaDeviceMapHost);

    for (int i = 0; i < 2; ++i) {
        size_t data_size = static_cast<size_t>(capacity_) * max_packet_size_;
        size_t len_size = static_cast<size_t>(capacity_) * sizeof(uint16_t);

        // Allocate pinned memory with MAPPED flag
        if (cudaHostAlloc(&host_data_buffers_[i], data_size, cudaHostAllocMapped) != cudaSuccess)
            throw std::runtime_error("CUDA HostAlloc Data failed");
        memset(host_data_buffers_[i], 0, data_size);

        if (cudaHostAlloc(&host_length_buffers_[i], len_size, cudaHostAllocMapped) != cudaSuccess)
            throw std::runtime_error("CUDA HostAlloc Length failed");
        memset(host_length_buffers_[i], 0, len_size);

        // Get device pointers for the mapped host memory
        if (cudaHostGetDevicePointer(&device_data_ptrs_[i], host_data_buffers_[i], 0) != cudaSuccess)
            throw std::runtime_error("Failed to get device pointer for data buffer");

        if (cudaHostGetDevicePointer(&device_length_ptrs_[i], host_length_buffers_[i], 0) != cudaSuccess)
            throw std::runtime_error("Failed to get device pointer for length buffer");
        
        LOG_DEBUG("Buffer {} allocated. Host: {:p}, Device Mapped: {:p}", i, (void*)host_data_buffers_[i], (void*)device_data_ptrs_[i]);
    }
}

PacketBuffer::~PacketBuffer() {
    LOG_INFO("Releasing Zero-Copy PacketBuffer resources...");
    cudaDeviceSynchronize();
    for (int i = 0; i < 2; ++i) {
        if (host_data_buffers_[i]) cudaFreeHost(host_data_buffers_[i]);
        if (host_length_buffers_[i]) cudaFreeHost(host_length_buffers_[i]);
    }
}

void PacketBuffer::swap() {
    write_idx_ = 1 - write_idx_;
}

} // namespace fuzzer
