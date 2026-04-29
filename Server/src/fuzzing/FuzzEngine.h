#ifndef FUZZ_ENGINE_H
#define FUZZ_ENGINE_H

#include "PacketBuffer.h"
#include "DpdkSender.h"
#include "IFuzzStrategy.h"
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>

namespace fuzzer {

struct FuzzStats {
    std::atomic<uint64_t> packets_sent{0};
    std::atomic<double> current_pps{0.0};
    std::atomic<bool> target_alive{true};
};

class FuzzEngine {
public:
    FuzzEngine(DpdkSender& dpdk_sender);
    ~FuzzEngine();

    /**
     * @brief Start the fuzzing process.
     * @param strategy The strategy to use
     * @param batch_size Number of packets per macro-batch
     */
    void start(std::unique_ptr<IFuzzStrategy> strategy, uint32_t batch_size = 65536);

    /**
     * @brief Stop the fuzzing process.
     */
    void stop();

    bool is_running() const { return running_; }
    
    FuzzStats& get_stats() { return stats_; }

private:
    void loop();

    DpdkSender& dpdk_sender_;
    std::unique_ptr<PacketBuffer> buffer_;
    std::unique_ptr<IFuzzStrategy> strategy_;
    uint32_t batch_size_ = 0;

    std::thread worker_thread_;
    std::atomic<bool> running_{false};
    
    cudaStream_t stream_;
    FuzzStats stats_;
};

} // namespace fuzzer

#endif // FUZZ_ENGINE_H
