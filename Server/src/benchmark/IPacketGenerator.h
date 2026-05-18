#ifndef I_PACKET_GENERATOR_H
#define I_PACKET_GENERATOR_H

#include <cstdint>
#include <string>

namespace fuzzer {

struct PacketGenConfig {
    uint8_t src_mac[6];
    uint8_t dest_mac[6];
    uint32_t src_ip;
    uint32_t dest_ip;
};

class IPacketGenerator {
public:
    virtual ~IPacketGenerator() = default;
    virtual void generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) = 0;
    virtual std::string get_name() const = 0;
    virtual int get_thread_count() const = 0;
    virtual bool is_gpu() const { return false; }
};

} // namespace fuzzer

#endif // I_PACKET_GENERATOR_H
