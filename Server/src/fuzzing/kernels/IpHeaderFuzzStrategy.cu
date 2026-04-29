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
    
    // Exhaustive Mutation: 65536 threads cover all combinations of Flags and Fragment Offset
    ip->fragment_offset = (uint16_t)idx; 
    
    ip->ttl = 64;
    ip->protocol = 17; // UDP (dummy payload)
    ip->src_ip = config.src_ip;
    ip->dest_ip = config.dest_ip;

    compute_ip_checksum(ip);

    // 3. Dummy Payload (8 bytes)
    uint64_t* payload = (uint64_t*)(pkt + sizeof(EthernetHeader) + sizeof(IPv4Header));
    *payload = 0xDEADBEEFCAFEBABE;

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
