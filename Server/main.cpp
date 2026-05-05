#include "HttpSrv.h"
#include "VMService.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "DpdkSender.h"
#include "FuzzService.h"
#include "routing/FuzzRoutes.h"
#include "routing/VMRoutes.h"
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

    LOG_INFO("FUZZER SERVER STARTED");

    // Signal handlers for os signal
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize DPDK
    fuzzer::DpdkSender dpdk_sender;
    
    // Auto-setup for Virtual environment if no args provided
    if (argc <= 1) {
        LOG_INFO("No DPDK arguments provided. Using default TAP configuration for VM fuzzing.");
        const char* auto_argv[] = {
            argv[0],
            "--vdev=net_tap0,iface=dpdk0",
            "--lcores=0",
            "--log-level=lib.eal:6", // Reduce spam from EAL
            "--"
        };
        int auto_argc = sizeof(auto_argv) / sizeof(auto_argv[0]);
        if (!dpdk_sender.init(auto_argc, const_cast<char**>(auto_argv))) {
            LOG_WARN("DPDK auto-initialization failed.");
        }
    } else {
        if (!dpdk_sender.init(argc, argv)) {
            LOG_WARN("DPDK could not be initialized with provided arguments.");
        }
    }

    // Load configuration
    ConfigManager::instance().load("config.json");

    VMService vm_service;
    fuzzer::FuzzService fuzz_service(dpdk_sender, vm_service);

    HttpSrv server("0.0.0.0", 8080);
    g_server = &server;

    server.add_route_group(std::make_unique<VMRoutes>(vm_service));
    server.add_route_group(std::make_unique<fuzzer::FuzzRoutes>(fuzz_service));

    LOG_INFO("Server started on port 8080");
    server.start();

    g_server = nullptr;
    LOG_INFO("Server closed");
    Logger::shutdown();
    return 0;
}
