#include "TelemetryRoutes.h"
#include "Logger.h"

namespace fuzzer {

// --- WebSocketBroadcaster Implementation ---

void WebSocketBroadcaster::add_connection(crow::websocket::connection* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.insert(conn);
    LOG_INFO("WebSocket: New client connected. Total clients: {}", connections_.size());
}

void WebSocketBroadcaster::remove_connection(crow::websocket::connection* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.erase(conn);
    LOG_INFO("WebSocket: Client disconnected. Total clients: {}", connections_.size());
}

void WebSocketBroadcaster::on_stats_update(const nlohmann::json& stats) {
    broadcast({
        {"type", "stats"},
        {"data", stats}
    });
}

void WebSocketBroadcaster::on_crash_detected(const std::string& vm_id) {
    broadcast({
        {"type", "crash"},
        {"vm_id", vm_id}
    });
}

void WebSocketBroadcaster::on_log_message(const std::string& level, const std::string& msg) {
    broadcast({
        {"type", "log"},
        {"level", level},
        {"message", msg}
    });
}

void WebSocketBroadcaster::broadcast(const nlohmann::json& message) {
    std::string text = message.dump();
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto conn : connections_) {
        try {
            conn->send_text(text);
        } catch (const std::exception& e) {
            LOG_ERROR("WebSocket: Failed to send message: {}", e.what());
        }
    }
}

// --- TelemetryRoutes Implementation ---

TelemetryRoutes::TelemetryRoutes(NotificationHub& hub) : hub_(hub) {
    broadcaster_ = std::make_shared<WebSocketBroadcaster>();
    hub_.add_listener(broadcaster_);
}

void TelemetryRoutes::register_routes(crow::SimpleApp& app) {
    CROW_WEBSOCKET_ROUTE(app, "/api/telemetry")
        .onopen([this](crow::websocket::connection& conn) {
            broadcaster_->add_connection(&conn);
        })
        .onclose([this](crow::websocket::connection& conn, const std::string& reason) {
            broadcaster_->remove_connection(&conn);
        })
        .onerror([this](crow::websocket::connection& conn, const std::string& msg) {
            broadcaster_->remove_connection(&conn);
        })
        .onmessage([this](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            LOG_DEBUG("WebSocket: Received message (ignored): {}", data);
        });
}

} // namespace fuzzer
