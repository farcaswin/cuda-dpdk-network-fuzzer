#include "FuzzRoutes.h"
#include "Logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fuzzer {

FuzzRoutes::FuzzRoutes(FuzzService& service) : service_(service) {}

void FuzzRoutes::register_routes(httplib::Server& svr) {
    svr.Post("/api/fuzz/start", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            FuzzParams params;
            params.vm_id = body.at("vm_id").get<std::string>();
            params.target_ip = body.at("target_ip").get<std::string>();
            params.target_mac = body.at("target_mac").get<std::string>();
            params.src_ip = body.value("src_ip", "10.10.10.1"); // Default host IP in fuzz-net
            params.src_mac = body.value("src_mac", "00:00:00:00:00:00"); 
            params.strategy = body.at("strategy").get<std::string>();
            params.batch_size = body.value("batch_size", 65536);
            params.auto_restore = body.value("auto_restore", false);

            service_.start_fuzzing(params);
            res.set_content(json{{"status", "success"}, {"message", "Fuzzing started"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to start fuzzing: {}", e.what());
            res.status = 400;
            res.set_content(json{{"status", "error"}, {"message", e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/api/fuzz/stop", [this](const httplib::Request& req, httplib::Response& res) {
        service_.stop_fuzzing();
        res.set_content(json{{"status", "success"}, {"message", "Fuzzing stopped"}}.dump(), "application/json");
    });

    svr.Get("/api/fuzz/status", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_content(service_.get_status().dump(), "application/json");
    });
}

} // namespace fuzzer
