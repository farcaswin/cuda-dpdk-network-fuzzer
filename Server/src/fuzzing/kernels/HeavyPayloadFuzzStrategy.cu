#include "HeavyPayloadFuzzStrategy.h"
#include "NetworkHeaders.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace fuzzer {

/**
 * @brief Simple Xorshift PRNG for GPU threads.
 * Extremely fast on GPU, but CPU has to do it sequentially.
 */
__device__ inline uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/**
 * @brief Simple CRC32 implementation for the GPU.
 */
__device__ inline uint32_t compute_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

__global__ void heavy_payload_kernel(uint8_t* data, 
                                     uint16_t* lengths, 
                                     uint32_t batch_size,
                                     HeavyPayloadFuzzStrategy::Config config) {
    uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= batch_size) return;

    uint8_t* pkt = data + (static_cast<size_t>(idx) * 1514);

    // 1. Headers (Ethernet + IP + UDP)
    EthernetHeader* eth = (EthernetHeader*)pkt;
    for (int i = 0; i < 6; ++i) {
        eth->dest_mac[i] = config.dest_mac[i];
        eth->src_mac[i] = config.src_mac[i];
    }
    eth->ether_type = swap_uint16(0x0800);

    IPv4Header* ip = (IPv4Header*)(pkt + 14);
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->id = swap_uint16((uint16_t)idx);
    ip->fragment_offset = 0;
    ip->protocol = 17; // UDP
    set_uint32(&ip->src_ip, config.src_ip);
    set_uint32(&ip->dest_ip, config.dest_ip);
    ip->ttl = 64;
    ip->total_length = swap_uint16(sizeof(IPv4Header) + sizeof(UdpHeader) + 1024); // IP + UDP + Payload
    
    compute_ip_checksum(ip);

    // 1.1 UDP Header
    UdpHeader* udp = (UdpHeader*)(pkt + 14 + sizeof(IPv4Header));
    set_uint16(&udp->src_port, swap_uint16(49152 + (idx % 16383)));
    set_uint16(&udp->dest_port, swap_uint16(445)); // Default target
    udp->length = swap_uint16(sizeof(UdpHeader) + 1024);
    udp->checksum = 0; // Optional in UDP, but good for fuzzing

    // 2. Heavy Computation: Payload Generation (1024 bytes)
    uint8_t* payload = pkt + 14 + sizeof(IPv4Header) + sizeof(UdpHeader);
    uint32_t prng_state = idx + 1; // Unique seed per thread
    
    // Fill 1024 bytes using PRNG
    for (int i = 0; i < 1024; i += 4) {
        uint32_t val = xorshift32(&prng_state);
        payload[i] = (uint8_t)(val & 0xFF);
        payload[i+1] = (uint8_t)((val >> 8) & 0xFF);
        payload[i+2] = (uint8_t)((val >> 16) & 0xFF);
        payload[i+3] = (uint8_t)((val >> 24) & 0xFF);
    }

    // 3. Heavy Computation: CRC32 of the payload
    // FIX: compute CRC over first 1020 bytes, then append — do not overwrite computed region
    uint32_t crc = compute_crc32(payload, 1020);
    
    // Put CRC at the end of payload (safe unaligned write)
    payload[1020] = (uint8_t)(crc & 0xFF);
    payload[1021] = (uint8_t)((crc >> 8) & 0xFF);
    payload[1022] = (uint8_t)((crc >> 16) & 0xFF);
    payload[1023] = (uint8_t)((crc >> 24) & 0xFF);

    lengths[idx] = 14 + sizeof(IPv4Header) + sizeof(UdpHeader) + 1024;
}

HeavyPayloadFuzzStrategy::HeavyPayloadFuzzStrategy(const Config& config) : config_(config) {}

void HeavyPayloadFuzzStrategy::launch_kernel(uint8_t* output_buffer, 
                                             uint16_t* lengths_buffer, 
                                             uint32_t batch_size, 
                                             cudaStream_t stream) {
    int threads_per_block = 256;
    int blocks_per_grid = (batch_size + threads_per_block - 1) / threads_per_block;
    
    heavy_payload_kernel<<<blocks_per_grid, threads_per_block, 0, stream>>>(
        output_buffer, 
        lengths_buffer, 
        batch_size,
        config_
    );
}

} // namespace fuzzer
