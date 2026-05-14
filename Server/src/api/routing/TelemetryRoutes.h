#ifndef TELEMETRY_ROUTES_H
#define TELEMETRY_ROUTES_H

#include "RouteGroup.h"
#include "NotificationHub.h"
#include <crow.h>
#include <mutex>
#include <unordered_set>
#include <memory>

namespace fuzzer {

/**
 * @brief Manages WebSocket connections and broadcasts notifications.
 */
class WebSocketBroadcaster : public INotificationListener {
public:
    void add_connection(crow::websocket::connection* conn);
    void remove_connection(crow::websocket::connection* conn);

    // INotificationListener implementation
    void on_stats_update(const nlohmann::json& stats) override;
    void on_crash_detected(const std::string& vm_id) override;
    void on_log_message(const std::string& level, const std::string& msg) override;

private:
    void broadcast(const nlohmann::json& message);

    std::mutex mutex_;
    std::unordered_set<crow::websocket::connection*> connections_;
};

/**
 * @brief Defines the /api/telemetry WebSocket route.
 */
class TelemetryRoutes : public RouteGroup {
public:
    explicit TelemetryRoutes(NotificationHub& hub);
    void register_routes(crow::SimpleApp& app) override;

private:
    NotificationHub& hub_;
    std::shared_ptr<WebSocketBroadcaster> broadcaster_;
};

} // namespace fuzzer

#endif // TELEMETRY_ROUTES_H
