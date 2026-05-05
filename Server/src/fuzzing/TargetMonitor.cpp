#include "TargetMonitor.h"
#include "Logger.h"
#include <chrono>
#include <cstdio>
#include <array>
#include <cstdlib>

namespace fuzzer {

TargetMonitor::TargetMonitor() {}

TargetMonitor::~TargetMonitor() {
    stop();
}

void TargetMonitor::start(const std::string& target_ip, const std::string& interface, std::function<void()> on_crash) {
    if (running_) return;
    
    running_ = true;
    thread_ = std::thread(&TargetMonitor::loop, this, target_ip, interface, on_crash);
    LOG_INFO("TargetMonitor started for {} on {}", target_ip, interface);
}

void TargetMonitor::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TargetMonitor::loop(const std::string& target_ip, const std::string& interface, std::function<void()> on_crash) {
    int failure_count = 0;
    
    while (running_) {
        // Use ping -c 1 -W 1 -I <iface> <ip>
        // -c 1: one packet
        // -W 1: timeout 1 second
        // -I: interface
        std::string cmd = "ping -c 1 -W 1 -I " + interface + " " + target_ip + " > /dev/null 2>&1";
        
        int res = std::system(cmd.c_str());
        
        if (res != 0) {
            failure_count++;
            LOG_WARN("Target {} not responding (failure {}/{})", target_ip, failure_count, max_failures_);
        } else {
            failure_count = 0;
        }

        if (failure_count >= max_failures_) {
            LOG_CRITICAL("TARGET CRASH DETECTED: {} is unreachable!", target_ip);
            if (on_crash) on_crash();
            running_ = false; // Stop monitoring after crash detection
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

} // namespace fuzzer
