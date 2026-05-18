#ifndef HEAVY_PAYLOAD_GENERATOR_H
#define HEAVY_PAYLOAD_GENERATOR_H

#include "IPacketGenerator.h"
#include <string>

namespace fuzzer {

class HeavyPayloadGenerator : public IPacketGenerator {
public:
    void generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) override;
    std::string get_name() const override { return "CPU Heavy Payload (PRNG+CRC32)"; }
    int get_thread_count() const override { return 1; }
};

} // namespace fuzzer

#endif // HEAVY_PAYLOAD_GENERATOR_H
