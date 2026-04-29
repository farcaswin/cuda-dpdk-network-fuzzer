#include "Logger.h"
#include "HttpSrv.h"
#include <iostream>
#include <csignal>

HttpSrv* g_server = nullptr;

void signal_handler(int signal) {
    if (g_server) {
        LOG_INFO("Signal {} received. Server will stop.", signal);
        g_server->stop();
    }
}

int main(int argc, char** argv) {
    Logger::init(
        "logs",
        "fuzzer-srv",
        spdlog::level::debug
    );

    LOG_INFO("Fuzzer Server Started");

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    HttpSrv server("0.0.0.0", 8080);
    g_server = &server;

    LOG_INFO("Server started on port 8080");
    server.start();

    g_server = nullptr;
    LOG_INFO("Server closed");
    Logger::shutdown();
    return 0;
}
