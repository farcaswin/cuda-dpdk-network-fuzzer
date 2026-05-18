#include "IpHeaderFuzzStrategy.h"
#include "NetworkHeaders.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace fuzzer {

__global__ void ip_header_fuzz_kernel(uint8_t* data, 
                                      uint16_t* lengths, 
                                      uint32_t batch_size,
                                      IpHeaderFuzzStrategy::Config config) {
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
    ip->total_length = swap_uint16(sizeof(IPv4Header) + 8); // Small payload
    ip->id = swap_uint16((uint16_t)idx);
    
    // Exhaustive Mutation of Flags (3 bits) and Fragment Offset (13 bits)
    // Bits 15-13: Flags (Reserved, DF, MF)
    // Bits 12-0: Fragment Offset (in 8-byte units)
    uint16_t flags = (uint16_t)((idx >> 13) & 0x0007);
    uint16_t offset = (uint16_t)(idx & 0x1FFF);
    uint16_t frag_field = (uint16_t)((flags << 13) | offset);
    
    ip->fragment_offset = swap_uint16(frag_field);
    
    ip->ttl = 64;
    ip->protocol = 17; // UDP (dummy payload)
    set_uint32(&ip->src_ip, config.src_ip);
    set_uint32(&ip->dest_ip, config.dest_ip);

    compute_ip_checksum(ip);

    // 3. Dummy Payload (8 bytes)
    set_uint64(pkt + sizeof(EthernetHeader) + sizeof(IPv4Header), 0xDEADBEEFCAFEBABE);

    lengths[idx] = sizeof(EthernetHeader) + sizeof(IPv4Header) + 8;
}

IpHeaderFuzzStrategy::IpHeaderFuzzStrategy(const Config& config) : config_(config) {}

void IpHeaderFuzzStrategy::launch_kernel(uint8_t* output_buffer, 
                                         uint16_t* lengths_buffer, 
                                         uint32_t batch_size, 
                                         cudaStream_t stream) {
    int threads_per_block = 256;
    int blocks_per_grid = (batch_size + threads_per_block - 1) / threads_per_block;
    
    ip_header_fuzz_kernel<<<blocks_per_grid, threads_per_block, 0, stream>>>(
        output_buffer, 
        lengths_buffer, 
        batch_size,
        config_
    );
}

} // namespace fuzzer
