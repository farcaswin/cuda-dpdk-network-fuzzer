#include "TcpFuzzStrategy.h"
#include "NetworkHeaders.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace fuzzer {

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
    tcp->src_port = swap_uint16(49152 + (idx % 16383)); // Ephemeral range
    
    // Target port (can be exhaustive or fixed via config)
    // If you want to scan all ports: tcp->dest_port = swap_uint16((uint16_t)idx);
    tcp->dest_port = swap_uint16(config.dest_port_base);
    
    tcp->seq_num = swap_uint32(idx);
    tcp->ack_num = 0;
    tcp->res1 = 0;
    tcp->data_offset = 5; // 20 bytes
    
    // Mutate Flags: 64 combinations (SYN, FIN, RST, PSH, ACK, URG)
    tcp->flags = (uint8_t)(idx & 0x3F); 
    
    tcp->window = swap_uint16(8192);
    tcp->urgent_ptr = 0;
    
    // 4. Compute correct TCP Checksum with pseudo-header
    compute_tcp_checksum(ip, tcp);
    
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
