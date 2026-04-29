#ifndef IP_HEADER_FUZZ_STRATEGY_H
#define IP_HEADER_FUZZ_STRATEGY_H

#include "IFuzzStrategy.h"
#include <string>

namespace fuzzer {

class IpHeaderFuzzStrategy : public IFuzzStrategy {
public:
    struct Config {
        uint8_t dest_mac[6];
        uint8_t src_mac[6];
        uint32_t dest_ip;
        uint32_t src_ip;
    };

    IpHeaderFuzzStrategy(const Config& config);
    virtual ~IpHeaderFuzzStrategy() = default;

    void launch_kernel(uint8_t* output_buffer, 
                       uint16_t* lengths_buffer, 
                       uint32_t batch_size, 
                       cudaStream_t stream) override;

    std::string get_name() const override { return "IP_HEADER_EXHAUSTIVE"; }

private:
    Config config_;
};

} // namespace fuzzer

#endif // IP_HEADER_FUZZ_STRATEGY_H
