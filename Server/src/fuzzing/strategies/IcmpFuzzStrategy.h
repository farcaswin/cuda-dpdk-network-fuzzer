#ifndef ICMP_FUZZ_STRATEGY_H
#define ICMP_FUZZ_STRATEGY_H

#include "IFuzzStrategy.h"
#include <string>
#include <vector>

namespace fuzzer {

class IcmpFuzzStrategy : public IFuzzStrategy {
public:
    struct Config {
        uint8_t dest_mac[6];
        uint8_t src_mac[6];
        uint32_t dest_ip;
        uint32_t src_ip;
    };

    IcmpFuzzStrategy(const Config& config);
    virtual ~IcmpFuzzStrategy() = default;

    void launch_kernel(uint8_t* output_buffer, 
                       uint16_t* lengths_buffer, 
                       uint32_t batch_size, 
                       cudaStream_t stream) override;

    std::string get_name() const override { return "ICMP_TYPES_EXHAUSTIVE"; }

private:
    Config config_;
};

} // namespace fuzzer

#endif // ICMP_FUZZ_STRATEGY_H
