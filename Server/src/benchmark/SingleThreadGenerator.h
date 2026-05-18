#ifndef SINGLE_THREAD_GENERATOR_H
#define SINGLE_THREAD_GENERATOR_H

#include "IPacketGenerator.h"

namespace fuzzer {

class SingleThreadGenerator : public IPacketGenerator {
public:
    void generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) override;
    std::string get_name() const override { return "CPU Single-Threaded"; }
    int get_thread_count() const override { return 1; }
};

} // namespace fuzzer

#endif // SINGLE_THREAD_GENERATOR_H
