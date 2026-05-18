#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include <cstdint>
#include <arpa/inet.h>

namespace fuzzer {

// Re-using structure definitions but without __device__
#pragma pack(push, 1)
struct EthHdr {
    uint8_t d_mac[6];
    uint8_t s_mac[6];
    uint16_t eth_type;
};

struct IpV4Hdr {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t proto;
    uint16_t check;
    uint32_t s_addr;
    uint32_t d_addr;
};

struct IcmpHdr {
    uint8_t type;
    uint8_t code;
    uint16_t check;
    uint16_t id;
    uint16_t seq;
};
#pragma pack(pop)

inline uint16_t cpu_checksum(const uint16_t* buf, int len) {
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

inline void generate_icmp_packet(uint8_t* pkt, uint32_t idx, const uint8_t* smac, const uint8_t* dmac, uint32_t sip, uint32_t dip) {
    EthHdr* eth = (EthHdr*)pkt;
    for (int i = 0; i < 6; ++i) {
        eth->s_mac[i] = smac[i];
        eth->d_mac[i] = dmac[i];
    }
    eth->eth_type = htons(0x0800);

    IpV4Hdr* ip = (IpV4Hdr*)(pkt + 14);
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(IpV4Hdr) + sizeof(IcmpHdr));
    ip->id = htons((uint16_t)idx);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->proto = 1;
    ip->s_addr = sip;
    ip->d_addr = dip;
    ip->check = 0;
    ip->check = cpu_checksum((uint16_t*)ip, sizeof(IpV4Hdr));

    IcmpHdr* icmp = (IcmpHdr*)(pkt + 14 + sizeof(IpV4Hdr));
    icmp->type = (uint8_t)(idx / 256);
    icmp->code = (uint8_t)(idx % 256);
    icmp->id = htons(0x1234);
    icmp->seq = htons((uint16_t)idx);
    icmp->check = 0;
    icmp->check = cpu_checksum((uint16_t*)icmp, sizeof(IcmpHdr));
}

} // namespace fuzzer

#endif // CPU_UTILS_H
