#include "Logger.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#include <vector>

std::shared_ptr<spdlog::logger> Logger::instance_ = nullptr;

void Logger::init(const std::string& log_dir, const std::string& app_name, spdlog::level::level_enum level){
    std::filesystem::create_directories(log_dir);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
    console_sink->set_level(level);
    console_sink->set_pattern(
        "%^[%H:%M:%S] [%l]%$ %v"
    );

    std::string log_path = log_dir + "/" + app_name + ".log";

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_path,
        10 * 1024 * 1024,   // 10 MB / file
        5                    // max 5 files
    );
    file_sink->set_level(spdlog::level::trace); 
    file_sink->set_pattern(
        "[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v"
    );

    std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };

    instance_ = std::make_shared<spdlog::logger>(app_name, 
                    sinks.begin(), sinks.end());
    instance_->set_level(spdlog::level::trace);

    instance_->flush_on(spdlog::level::err);

    spdlog::register_logger(instance_);
    spdlog::set_default_logger(instance_);

    instance_->info("Logger initializat — fisier: {}", log_path);
}

std::shared_ptr<spdlog::logger> Logger::get(){
    if (!instance_){
        throw std::runtime_error("Logger init() not called");
    }
    return instance_;
}

void Logger::shutdown(){
    spdlog::shutdown();
}