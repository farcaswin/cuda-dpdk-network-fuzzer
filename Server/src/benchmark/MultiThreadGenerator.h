#ifndef MULTI_THREAD_GENERATOR_H
#define MULTI_THREAD_GENERATOR_H

#include "IPacketGenerator.h"
#include <thread>

namespace fuzzer {

class MultiThreadGenerator : public IPacketGenerator {
public:
    MultiThreadGenerator(int thread_count = 0);
    void generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) override;
    std::string get_name() const override { return "CPU Multi-Threaded (" + std::to_string(thread_count_) + " threads)"; }
    int get_thread_count() const override { return thread_count_; }

private:
    int thread_count_;
};

} // namespace fuzzer

#endif // MULTI_THREAD_GENERATOR_H
