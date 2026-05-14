#include "FuzzRoutes.h"
#include "Logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fuzzer {

FuzzRoutes::FuzzRoutes(FuzzService& service) : service_(service) {}

void FuzzRoutes::register_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/fuzz/start").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        crow::response res;
        res.add_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        try {
            auto body = json::parse(req.body);
            FuzzParams params;
            params.vm_id = body.at("vm_id").get<std::string>();
            params.target_ip = body.at("target_ip").get<std::string>();
            params.target_mac = body.at("target_mac").get<std::string>();
            params.src_ip = body.value("src_ip", "10.10.10.1"); // Default host IP in fuzz-net
            params.src_mac = body.value("src_mac", "00:11:22:33:44:55"); // Valid dummy source MAC
            params.strategy = body.at("strategy").get<std::string>();
            params.target_port = body.value("target_port", 445); // Default to 445 for TCP fuzzing
            params.batch_size = body.value("batch_size", 65536);
            params.rate_pps = body.value("rate_pps", 0);
            params.duration_sec = body.value("duration_sec", 0);
            params.auto_restore = body.value("auto_restore", false);
            service_.start_fuzzing(params);
            res.body = json{{"status", "success"}, {"message", "Fuzzing started"}}.dump();
            res.code = 200;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to start fuzzing: {}", e.what());
            res.code = 400;
            res.body = json{{"status", "error"}, {"message", e.what()}}.dump();
        }
        return res;
    });

    CROW_ROUTE(app, "/api/fuzz/stop").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        service_.stop_fuzzing();
        crow::response res(json{{"status", "success"}, {"message", "Fuzzing stopped"}}.dump());
        res.add_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/fuzz/status").methods(crow::HTTPMethod::GET)
    ([this](const crow::request& req) {
        crow::response res(service_.get_status().dump());
        res.add_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        return res;
    });
}

} // namespace fuzzer
