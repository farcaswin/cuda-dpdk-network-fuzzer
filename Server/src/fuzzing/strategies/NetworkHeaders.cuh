#ifndef NETWORK_HEADERS_CUH
#define NETWORK_HEADERS_CUH

#include <cstdint>
#include <cuda_runtime.h>

namespace fuzzer {

#pragma pack(push, 1)

struct EthernetHeader {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ether_type;
};

struct IPv4Header {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t fragment_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
};

struct IcmpHeader {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
};

struct TcpHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t res1 : 4;
    uint8_t data_offset : 4;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
};

#pragma pack(pop)

/**
 * @brief RFC 1071 Checksum computed on the GPU.
 * Safe for unaligned buffers.
 */
__device__ inline uint16_t calculate_checksum(const void* buf, int len) {
    uint32_t sum = 0;
    const uint8_t* p = (const uint8_t*)buf;
    
    while (len > 1) {
        uint16_t val;
        val = p[0] | (p[1] << 8);
        sum += val;
        p += 2;
        len -= 2;
    }
    if (len == 1) {
        sum += *p;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)(~sum);
}

/**
 * @brief Safe unaligned write for 16-bit values.
 */
__device__ inline void set_uint16(void* ptr, uint16_t val) {
    uint8_t* p = (uint8_t*)ptr;
    p[0] = (uint8_t)(val & 0xFF);
    p[1] = (uint8_t)(val >> 8);
}

/**
 * @brief Safe unaligned write for 32-bit values.
 */
__device__ inline void set_uint32(void* ptr, uint32_t val) {
    uint8_t* p = (uint8_t*)ptr;
    p[0] = (uint8_t)(val & 0xFF);
    p[1] = (uint8_t)((val >> 8) & 0xFF);
    p[2] = (uint8_t)((val >> 16) & 0xFF);
    p[3] = (uint8_t)((val >> 24) & 0xFF);
}

/**
 * @brief Safe unaligned write for 64-bit values.
 */
__device__ inline void set_uint64(void* ptr, uint64_t val) {
    uint8_t* p = (uint8_t*)ptr;
    for (int i = 0; i < 8; ++i) {
        p[i] = (uint8_t)((val >> (i * 8)) & 0xFF);
    }
}

/**
 * @brief Helper for IP checksum (just header).
 */
__device__ inline void compute_ip_checksum(IPv4Header* ip) {
    ip->checksum = 0;
    ip->checksum = calculate_checksum(ip, sizeof(IPv4Header));
}

__device__ inline uint16_t swap_uint16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

__device__ inline uint32_t swap_uint32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

/**
 * @brief Calculează checksum-ul TCP (include pseudo-header).
 * Adaptat pentru accese safe (nealiniate).
 */
__device__ inline void compute_tcp_checksum(IPv4Header* ip, TcpHeader* tcp) {
    tcp->checksum = 0;
    uint32_t sum = 0;

    // Pseudo-header (RFC 793)
    // Source & Dest IP
    const uint8_t* src_ptr = (const uint8_t*)&ip->src_ip;
    const uint8_t* dest_ptr = (const uint8_t*)&ip->dest_ip;

    sum += src_ptr[0] | (src_ptr[1] << 8);
    sum += src_ptr[2] | (src_ptr[3] << 8);
    sum += dest_ptr[0] | (dest_ptr[1] << 8);
    sum += dest_ptr[2] | (dest_ptr[3] << 8);

    // Protocol (6) și Zero
    sum += swap_uint16((uint16_t)ip->protocol); 

    // TCP Length
    uint16_t tcp_len = sizeof(TcpHeader); 
    sum += swap_uint16(tcp_len);

    // Header-ul TCP
    const uint8_t* tcp_raw = (const uint8_t*)tcp;
    for (int i = 0; i < sizeof(TcpHeader); i += 2) {
        sum += tcp_raw[i] | (tcp_raw[i+1] << 8);
    }

    // Folding
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    tcp->checksum = (uint16_t)(~sum);
}


} // namespace fuzzer

#endif // NETWORK_HEADERS_CUH
