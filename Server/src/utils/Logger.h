#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <memory>
#include <string>

class Logger{
public: 
    static void init(const std::string& log_dir = "logs",
                    const std::string& app_name = "fuzzer_srv",
                    spdlog::level::level_enum level = spdlog::level::info);
                
    static std::shared_ptr<spdlog::logger> get();
    static void shutdown();
private:
    Logger() = delete;
    static std::shared_ptr<spdlog::logger> instance_;
};

#define LOG_TRACE(...)    Logger::get()->trace   (__VA_ARGS__)
#define LOG_DEBUG(...)    Logger::get()->debug   (__VA_ARGS__)
#define LOG_INFO(...)     Logger::get()->info    (__VA_ARGS__)
#define LOG_WARN(...)     Logger::get()->warn    (__VA_ARGS__)
#define LOG_ERROR(...)    Logger::get()->error   (__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::get()->critical(__VA_ARGS__)