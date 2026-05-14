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
 * @brief RFC 1071 Checksum computed on the GPU
 */
__device__ inline uint16_t calculate_checksum(const uint16_t* buf, int len) {
    uint32_t sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(const uint8_t*)buf;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)(~sum);
}

/**
 * @brief Helper for IP checksum (just header).
 */
__device__ inline void compute_ip_checksum(IPv4Header* ip) {
    ip->checksum = 0;
    ip->checksum = calculate_checksum((uint16_t*)ip, sizeof(IPv4Header));
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
 */
__device__ inline void compute_tcp_checksum(IPv4Header* ip, TcpHeader* tcp) {
    tcp->checksum = 0;

    // Pseudo-header (RFC 793)
    // | Source Address (4) | Dest Address (4) | Zero (1) | Proto (1) | TCP Length (2) |
    uint32_t sum = 0;

    // Adăugăm adresele IP (ca uint16_t x 4)
    uint16_t* src_ptr = (uint16_t*)&ip->src_ip;
    uint16_t* dest_ptr = (uint16_t*)&ip->dest_ip;

    sum += src_ptr[0];
    sum += src_ptr[1];
    sum += dest_ptr[0];
    sum += dest_ptr[1];

    // Protocol (6) și Zero
    sum += swap_uint16(ip->protocol); 

    // TCP Length (Header + Data). Aici avem doar Header (20 bytes de obicei)
    uint16_t tcp_len = swap_uint16(sizeof(TcpHeader)); 
    sum += tcp_len;

    // Adăugăm header-ul TCP
    uint16_t* tcp_ptr = (uint16_t*)tcp;
    for (int i = 0; i < sizeof(TcpHeader) / 2; i++) {
        sum += tcp_ptr[i];
    }

    // Folding
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    tcp->checksum = (uint16_t)(~sum);
}


} // namespace fuzzer

#endif // NETWORK_HEADERS_CUH
