#include "FuzzEngine.h"
#include "Logger.h"
#include <chrono>

namespace fuzzer {

FuzzEngine::FuzzEngine(DpdkSender& dpdk_sender, NotificationHub& hub) 
    : dpdk_sender_(dpdk_sender), hub_(hub) {
    cudaStreamCreate(&stream_);
}

FuzzEngine::~FuzzEngine() {
    stop();
    cudaStreamDestroy(stream_);
}

void FuzzEngine::start(std::unique_ptr<IFuzzStrategy> strategy, uint32_t batch_size, uint32_t rate_pps, uint32_t duration_sec) {
    if (running_) return;

    strategy_ = std::move(strategy);
    batch_size_ = batch_size;
    rate_pps_ = rate_pps;
    duration_sec_ = duration_sec;
    buffer_ = std::make_unique<PacketBuffer>(batch_size_);
    
    // Reset stats for new session
    stats_.packets_sent = 0;
    stats_.current_pps = 0.0;
    stats_.target_alive = true;

    running_ = true;
    worker_thread_ = std::thread(&FuzzEngine::loop, this);
    
    LOG_INFO("FuzzEngine started with strategy: {}", strategy_->get_name());
}

void fuzzer::FuzzEngine::stop() {
    if (!running_) return;

    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    strategy_.reset();
    buffer_.reset();
    LOG_INFO("FuzzEngine stopped.");
}

void FuzzEngine::loop() {
    bool has_previous_data = false;
    auto start_time = std::chrono::steady_clock::now();
    uint64_t total_sent_in_period = 0;
    auto last_stats_time = start_time;

    // Time budget for one batch in microseconds
    long long batch_time_us = 0;
    if (rate_pps_ > 0) {
        batch_time_us = (static_cast<long long>(batch_size_) * 1000000) / rate_pps_;
        LOG_INFO("Rate limiting enabled: {} PPS (target batch time: {} us)", rate_pps_, batch_time_us);
    }

    while (running_) {
        auto iter_start = std::chrono::steady_clock::now();

        // 1. Launch CUDA kernel on the GPU (asynchronous)
        // It writes to the current 'write' buffer in PacketBuffer
        strategy_->launch_kernel(
            buffer_->get_gpu_data(),
            buffer_->get_gpu_lengths(),
            batch_size_,
            stream_
        );

        // 2. While GPU is busy, CPU sends the previous batch through DPDK
        if (has_previous_data) {
            uint32_t sent = dpdk_sender_.send_burst(
                buffer_->get_dpdk_data(),
                buffer_->get_dpdk_lengths(),
                batch_size_
            );
            stats_.packets_sent += sent;
            total_sent_in_period += sent;
        }

        // 3. Synchronize with GPU - wait for the current batch generation to finish
        cudaStreamSynchronize(stream_);

        // 4. Swap buffers for the next iteration
        buffer_->swap();
        has_previous_data = true;

        // 5. Rate limiting (Throttling)
        if (rate_pps_ > 0) {
            auto iter_end = std::chrono::steady_clock::now();
            auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(iter_end - iter_start).count();
            if (elapsed_us < batch_time_us) {
                std::this_thread::sleep_for(std::chrono::microseconds(batch_time_us - elapsed_us));
            }
        }

        // 6. Update PPS stats every 1 second
        auto now = std::chrono::steady_clock::now();

        // Check session duration
        if (duration_sec_ > 0) {
            auto elapsed_total = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            if (elapsed_total >= duration_sec_) {
                LOG_INFO("Session duration reached ({}s). Stopping fuzzing...", duration_sec_);
                running_ = false;
                break;
            }
        }

        auto elapsed_stats = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_stats_time).count();
        if (elapsed_stats >= 1000) {
            double pps = (total_sent_in_period * 1000.0) / elapsed_stats;
            stats_.current_pps = pps;
            
            LOG_DEBUG("Fuzzing: {:.2f} pps, Total: {}", pps, stats_.packets_sent.load());
            
            // Push stats to NotificationHub
            hub_.send_stats({
                {"pps", pps},
                {"packets_sent", stats_.packets_sent.load()},
                {"target_alive", stats_.target_alive.load()}
            });
            
            total_sent_in_period = 0;
            last_stats_time = now;
        }
    }
}

} // namespace fuzzer
