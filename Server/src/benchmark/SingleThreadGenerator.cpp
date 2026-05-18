#include "SingleThreadGenerator.h"
#include "CpuUtils.h"

namespace fuzzer {

void SingleThreadGenerator::generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) {
    for (uint32_t i = 0; i < batch_size; ++i) {
        uint8_t* pkt = output + (static_cast<size_t>(i) * 1514);
        generate_icmp_packet(pkt, i, config.src_mac, config.dest_mac, config.src_ip, config.dest_ip);
        lengths[i] = 14 + sizeof(IpV4Hdr) + sizeof(IcmpHdr);
    }
}

} // namespace fuzzer
