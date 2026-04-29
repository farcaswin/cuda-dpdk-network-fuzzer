#ifndef I_FUZZ_STRATEGY_H
#define I_FUZZ_STRATEGY_H

#include <cstdint>
#include <cuda_runtime.h>
#include <string>

namespace fuzzer {

/**
 * @brief Abstract interface for different fuzzing strategies.
 * Decouples the fuzzing logic (kernels) from the FuzzEngine.
 */
class IFuzzStrategy {
public:
    virtual ~IFuzzStrategy() = default;

    /**
     * @brief Launches the CUDA kernel to generate a batch of packets.
     * @param output_buffer Pinned memory buffer to store packet data
     * @param lengths_buffer Pinned memory buffer to store packet lengths
     * @param batch_size Number of packets to generate
     * @param stream CUDA stream to use for asynchronous execution
     */
    virtual void launch_kernel(uint8_t* output_buffer, 
                               uint16_t* lengths_buffer, 
                               uint32_t batch_size, 
                               cudaStream_t stream) = 0;

    /**
     * @brief Returns the name of the strategy.
     */
    virtual std::string get_name() const = 0;
};

} // namespace fuzzer

#endif // I_FUZZ_STRATEGY_H
