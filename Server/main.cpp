#include "HttpSrv.h"
#include "routing/VMRoutes.h"
#include "VMService.h"
#include "ConfigManager.h"
#include "Logger.h"
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

    LOG_INFO("FUZZER SERVER VM ORCHESTRATION STARTED");

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Load configuration
    ConfigManager::instance().load("config.json");

    VMService vm_service;

    HttpSrv server("0.0.0.0", 8080);
    g_server = &server;

    server.add_route_group(std::make_unique<VMRoutes>(vm_service));

    LOG_INFO("Server started on port 8080");
    server.start();

    g_server = nullptr;
    LOG_INFO("Server closed");
    Logger::shutdown();
    return 0;
}
