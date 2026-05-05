#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace fuzzer {

class TargetMonitor {
public:
    TargetMonitor();
    ~TargetMonitor();

    /**
     * @brief Start monitoring the target.
     * @param target_ip IP to ping
     * @param interface Network interface to use (e.g. virbr1)
     * @param on_crash Callback when target is down
     */
    void start(const std::string& target_ip, const std::string& interface, std::function<void()> on_crash);
    
    /**
     * @brief Stop monitoring.
     */
    void stop();

    bool is_running() const { return running_; }

private:
    void loop(const std::string& target_ip, const std::string& interface, std::function<void()> on_crash);
    
    std::thread thread_;
    std::atomic<bool> running_{false};
    int interval_ms_ = 500;
    int max_failures_ = 3;
};

} // namespace fuzzer
