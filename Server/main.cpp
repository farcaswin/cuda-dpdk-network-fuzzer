#include "Logger.h"
#include <iostream>

int main(int argc, char** argv) {
    Logger::init(
        "logs",
        "fuzzer-srv",
        spdlog::level::debug
    );

    LOG_INFO("Fuzzer Server Started");
    std::cout << "Hello World - FuzzerServer Initialized" << std::endl;

    LOG_INFO("Server closing");
    Logger::shutdown();
    return 0;
}
