#include "HttpSrv.h"
#include "Logger.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

HttpSrv::HttpSrv(const std::string& host, int port) : host_(host), port_(port) {
    app_.loglevel(crow::LogLevel::Warning);
}

void HttpSrv::add_route_group(std::unique_ptr<RouteGroup> group){
    route_groups_.push_back(std::move(group));
}

void HttpSrv::setup_global_handlers(){
    // CORS & OPTIONS support
    // In a real app we'd use middleware, but for this migration we'll keep it simple
    // Crow doesn't have a direct "pre-routing" hook in SimpleApp, so we'll 
    // rely on each route adding headers if needed, or we can use a catch-all for OPTIONS.
    
    CROW_ROUTE(app_, "/api/system/health")
    ([]() {
        crow::response res(nlohmann::json{{"status", "ok"}}.dump() + "\n");
        res.set_header("Content-Type", "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    CROW_ROUTE(app_, "/api/system/shutdown").methods(crow::HTTPMethod::POST)
    ([this]() {
        LOG_INFO("Shutdown request received via API");
        
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            this->stop();
        }).detach();

        crow::response res(nlohmann::json{{"status", "ok"}, {"message", "Server shutting down"}}.dump(4) + "\n");
        res.set_header("Content-Type", "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
        return res;
    });
}

// Start
void HttpSrv::start(){
    setup_global_handlers();

    for (auto& group : route_groups_){
        group->register_routes(app_);
    }

    app_.bindaddr(host_).port(port_).multithreaded().run();
}

void HttpSrv::stop(){   
    app_.stop();
}
