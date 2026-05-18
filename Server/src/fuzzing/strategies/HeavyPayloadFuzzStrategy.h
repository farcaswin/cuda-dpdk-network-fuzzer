#ifndef HEAVY_PAYLOAD_FUZZ_STRATEGY_H
#define HEAVY_PAYLOAD_FUZZ_STRATEGY_H

#include "IFuzzStrategy.h"
#include <string>

namespace fuzzer {

class HeavyPayloadFuzzStrategy : public IFuzzStrategy {
public:
    struct Config {
        uint8_t dest_mac[6];
        uint8_t src_mac[6];
        uint32_t dest_ip;
        uint32_t src_ip;
    };

    HeavyPayloadFuzzStrategy(const Config& config);
    virtual ~HeavyPayloadFuzzStrategy() = default;

    void launch_kernel(uint8_t* output_buffer, 
                       uint16_t* lengths_buffer, 
                       uint32_t batch_size, 
                       cudaStream_t stream) override;

    std::string get_name() const override { return "COMPUTE_HEAVY_PAYLOAD"; }

private:
    Config config_;
};

} // namespace fuzzer

#endif // HEAVY_PAYLOAD_FUZZ_STRATEGY_H
