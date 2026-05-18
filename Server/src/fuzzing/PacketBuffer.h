#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include <cstdint>
#include <cuda_runtime.h>
#include <vector>

namespace fuzzer {

/**
 * @brief Manages double-buffered pinned memory for high-speed packet generation.
 * Uses a pre-allocated pool to avoid runtime allocation failures.
 */
class PacketBuffer {
public:
    PacketBuffer(uint32_t max_batch_size, uint32_t max_packet_size = 1514);
    ~PacketBuffer();

    PacketBuffer(const PacketBuffer&) = delete;
    PacketBuffer& operator=(const PacketBuffer&) = delete;

    void swap();
    void set_batch_size(uint32_t batch_size) { 
        if (batch_size <= capacity_) current_batch_size_ = batch_size; 
    }

    // GPU/Device Access (mapped pointers)
    uint8_t* get_device_data() { return device_data_ptrs_[write_idx_]; }
    uint16_t* get_device_lengths() { return device_length_ptrs_[write_idx_]; }

    // Host Access for monitoring or debugging
    uint8_t* get_host_data() { return host_data_buffers_[write_idx_]; }
    uint16_t* get_host_lengths() { return host_length_buffers_[write_idx_]; }

    // DPDK Access (current read target)
    const uint8_t* get_dpdk_data() const { return host_data_buffers_[1 - write_idx_]; }
    const uint16_t* get_dpdk_lengths() const { return host_length_buffers_[1 - write_idx_]; }

    uint32_t get_batch_size() const { return current_batch_size_; }
    uint32_t get_capacity() const { return capacity_; }
    uint32_t get_max_packet_size() const { return max_packet_size_; }

private:
    uint32_t capacity_;
    uint32_t current_batch_size_;
    uint32_t max_packet_size_;
    uint8_t write_idx_ = 0; 

    uint8_t* host_data_buffers_[2] = {nullptr, nullptr};
    uint16_t* host_length_buffers_[2] = {nullptr, nullptr};
    
    // Mapped device pointers for the host buffers
    uint8_t* device_data_ptrs_[2] = {nullptr, nullptr};
    uint16_t* device_length_ptrs_[2] = {nullptr, nullptr};
};

} // namespace fuzzer

#endif // PACKET_BUFFER_H
