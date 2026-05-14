#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace fuzzer {

/**
 * @brief Interface for objects that want to listen to system events.
 */
class INotificationListener {
public:
    virtual ~INotificationListener() = default;
    
    virtual void on_stats_update(const nlohmann::json& stats) = 0;
    virtual void on_crash_detected(const std::string& vm_id) = 0;
    virtual void on_log_message(const std::string& level, const std::string& msg) = 0;
};

/**
 * @brief Thread-safe hub that manages listeners and broadcasts notifications.
 */
class NotificationHub {
public:
    void add_listener(std::shared_ptr<INotificationListener> listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.push_back(listener);
    }

    void send_stats(const nlohmann::json& stats) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& listener : listeners_) {
            listener->on_stats_update(stats);
        }
    }

    void send_crash(const std::string& vm_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& listener : listeners_) {
            listener->on_crash_detected(vm_id);
        }
    }

    void send_log(const std::string& level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& listener : listeners_) {
            listener->on_log_message(level, msg);
        }
    }

private:
    std::vector<std::shared_ptr<INotificationListener>> listeners_;
    std::mutex mutex_;
};

} // namespace fuzzer
