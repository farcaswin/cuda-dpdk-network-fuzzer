#include "TcpFuzzStrategy.h"
#include "NetworkHeaders.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace fuzzer {

struct TcpPseudoHeader {
    uint32_t src_ip;
    uint32_t dest_ip;
    uint8_t zero;
    uint8_t protocol;
    uint16_t tcp_length;
};

__global__ void tcp_fuzz_kernel(uint8_t* data, 
                                uint16_t* lengths, 
                                uint32_t batch_size,
                                TcpFuzzStrategy::Config config) {
    uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= batch_size) return;

    uint8_t* pkt = data + (static_cast<size_t>(idx) * 1514);

    // 1. Ethernet Header
    EthernetHeader* eth = (EthernetHeader*)pkt;
    for (int i = 0; i < 6; ++i) {
        eth->dest_mac[i] = config.dest_mac[i];
        eth->src_mac[i] = config.src_mac[i];
    }
    eth->ether_type = swap_uint16(0x0800);

    // 2. IPv4 Header
    IPv4Header* ip = (IPv4Header*)(pkt + sizeof(EthernetHeader));
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->total_length = swap_uint16(sizeof(IPv4Header) + sizeof(TcpHeader));
    ip->id = swap_uint16((uint16_t)idx);
    ip->fragment_offset = 0;
    ip->ttl = 64;
    ip->protocol = 6; // TCP
    ip->src_ip = config.src_ip;
    ip->dest_ip = config.dest_ip;

    compute_ip_checksum(ip);

    // 3. TCP Header
    TcpHeader* tcp = (TcpHeader*)(pkt + sizeof(EthernetHeader) + sizeof(IPv4Header));
    tcp->src_port = swap_uint16(12345);
    
    // Mutate dest port (all 65535 ports in one batch if batch_size is 65536)
    tcp->dest_port = swap_uint16((uint16_t)idx);
    
    tcp->seq_num = swap_uint32(idx);
    tcp->ack_num = 0;
    tcp->res1 = 0;
    tcp->data_offset = 5; // 20 bytes
    
    // Mutate Flags: 65536 / 1024 = 64 combinations of flags. 
    // We can just use (idx / 1024) for flags, or any other mapping.
    // Let's use bits 0-5 of idx for flags to ensure all 64 are covered multiple times.
    tcp->flags = (uint8_t)(idx & 0x3F); 
    
    tcp->window = swap_uint16(8192);
    tcp->urgent_ptr = 0;
    
    // TCP Checksum (simplified for demo, usually needs pseudo-header)
    tcp->checksum = 0;
    
    lengths[idx] = sizeof(EthernetHeader) + sizeof(IPv4Header) + sizeof(TcpHeader);
}

TcpFuzzStrategy::TcpFuzzStrategy(const Config& config) : config_(config) {}

void TcpFuzzStrategy::launch_kernel(uint8_t* output_buffer, 
                                    uint16_t* lengths_buffer, 
                                    uint32_t batch_size, 
                                    cudaStream_t stream) {
    int threads_per_block = 256;
    int blocks_per_grid = (batch_size + threads_per_block - 1) / threads_per_block;
    
    tcp_fuzz_kernel<<<blocks_per_grid, threads_per_block, 0, stream>>>(
        output_buffer, 
        lengths_buffer, 
        batch_size,
        config_
    );
}

} // namespace fuzzer
