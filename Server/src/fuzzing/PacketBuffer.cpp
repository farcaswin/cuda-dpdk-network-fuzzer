#include "PacketBuffer.h"
#include "Logger.h"
#include <stdexcept>

namespace fuzzer {

PacketBuffer::PacketBuffer(uint32_t batch_size, uint32_t max_packet_size)
    : batch_size_(batch_size), max_packet_size_(max_packet_size) {
    
    LOG_INFO("Allocating PacketBuffer: batch_size={}, max_packet_size={}, total_memory={} MB", 
             batch_size, max_packet_size, (2 * batch_size * max_packet_size) / (1024 * 1024));

    for (int i = 0; i < 2; ++i) {
        // Allocate data buffer (pinned memory)
        size_t data_size = static_cast<size_t>(batch_size_) * max_packet_size_;
        cudaError_t err = cudaMallocHost(&data_buffers_[i], data_size);
        if (err != cudaSuccess) {
            LOG_ERROR("Failed to allocate pinned data buffer {}: {}", i, cudaGetErrorString(err));
            throw std::runtime_error("CUDA Pinned Memory allocation failed");
        }

        // Allocate length buffer (pinned memory)
        size_t len_size = static_cast<size_t>(batch_size_) * sizeof(uint16_t);
        err = cudaMallocHost(&length_buffers_[i], len_size);
        if (err != cudaSuccess) {
            LOG_ERROR("Failed to allocate pinned length buffer {}: {}", i, cudaGetErrorString(err));
            throw std::runtime_error("CUDA Pinned Memory allocation failed");
        }
        
        LOG_DEBUG("Buffer {} allocated at {:p} (data) and {:p} (lengths)", i, (void*)data_buffers_[i], (void*)length_buffers_[i]);
    }
}

PacketBuffer::~PacketBuffer() {
    LOG_INFO("Releasing PacketBuffer resources...");
    for (int i = 0; i < 2; ++i) {
        if (data_buffers_[i]) cudaFreeHost(data_buffers_[i]);
        if (length_buffers_[i]) cudaFreeHost(length_buffers_[i]);
    }
}

void PacketBuffer::swap() {
    write_idx_ = 1 - write_idx_;
}

} // namespace fuzzer
