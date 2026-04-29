#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include <cstdint>
#include <cuda_runtime.h>
#include <vector>

namespace fuzzer {

/**
 * @brief Manages double-buffered pinned memory for high-speed packet generation.
 * Uses cudaMallocHost for zero-copy access and DMA efficiency.
 */
class PacketBuffer {
public:
    /**
     * @brief Constructor
     * @param batch_size Number of packets per buffer (e.g., 65536)
     * @param max_packet_size Max size per packet (e.g., 1514)
     */
    PacketBuffer(uint32_t batch_size, uint32_t max_packet_size = 1514);
    ~PacketBuffer();

    // Prevent copying
    PacketBuffer(const PacketBuffer&) = delete;
    PacketBuffer& operator=(const PacketBuffer&) = delete;

    /**
     * @brief Swaps the GPU (write) and DPDK (read) buffers.
     */
    void swap();

    // GPU Access (current write target)
    uint8_t* get_gpu_data() { return data_buffers_[write_idx_]; }
    uint16_t* get_gpu_lengths() { return length_buffers_[write_idx_]; }

    // DPDK Access (current read target)
    const uint8_t* get_dpdk_data() const { return data_buffers_[1 - write_idx_]; }
    const uint16_t* get_dpdk_lengths() const { return length_buffers_[1 - write_idx_]; }

    uint32_t get_batch_size() const { return batch_size_; }
    uint32_t get_max_packet_size() const { return max_packet_size_; }

private:
    uint32_t batch_size_;
    uint32_t max_packet_size_;
    uint8_t write_idx_ = 0; // 0 or 1

    // 2 buffers for data, 2 for lengths
    uint8_t* data_buffers_[2] = {nullptr, nullptr};
    uint16_t* length_buffers_[2] = {nullptr, nullptr};
};

} // namespace fuzzer

#endif // PACKET_BUFFER_H
