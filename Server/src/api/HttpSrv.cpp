#include "HttpSrv.h"
#include "Logger.h"
#include <iostream>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>

HttpSrv::HttpSrv(const std::string& host, int port) : host_(host), port_(port) {}

void HttpSrv::add_route_group(std::unique_ptr<RouteGroup> group){
    route_groups_.push_back(std::move(group));
}

void HttpSrv::setup_global_handlers(){
    server_.set_pre_routing_handler(
        [](const httplib::Request& req, httplib::Response& res)
        {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type");

            if (req.method == "OPTIONS"){
                res.status = 204;
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        } 
    );

    // Shutdown API
    server_.Post("/api/system/shutdown", [this](const httplib::Request&, httplib::Response& res) {
        LOG_INFO("Shutdown request received via API");
        res.status = 200;
        res.set_content(nlohmann::json{{"status", "ok"}, {"message", "Server shutting down"}}.dump(4) + "\n", "application/json");
        
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            this->stop();
        }).detach();
    });

    // 404
    server_.set_error_handler(
        [](const httplib::Request& req, httplib::Response& res)
        {
            nlohmann::json err = {
                {"error", "Not found"},
                {"path", req.path}
            };
            res.status = 404;
            res.set_content(err.dump(4) + "\n", "application/json");
        }
    );

}

// Start
void HttpSrv::start(){
    setup_global_handlers();

    for (auto& group : route_groups_){
        group->register_routes(server_);
    }

    server_.listen(host_.c_str(), port_);
}

void HttpSrv::stop(){   
    server_.stop();
}