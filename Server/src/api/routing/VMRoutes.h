#pragma once
#include "VMService.h"
#include "RouteGroup.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class VMRoutes : public RouteGroup {
public:
    explicit VMRoutes(VMService& vm_srv);
    void register_routes(crow::SimpleApp& app) override;

private:
    VMService& vm_service_;

    // Handlers
    crow::response handle_get_profiles(const crow::request&);
    crow::response handle_create_profile(const crow::request&);
    crow::response handle_update_profile(const crow::request&, std::string id);
    crow::response handle_list_available_disks(const crow::request&);
    crow::response handle_define_vm(const crow::request&, std::string id);
    crow::response handle_start_vm(const crow::request&, std::string id);
    crow::response handle_stop_vm(const crow::request&, std::string id);
    crow::response handle_get_status(const crow::request&, std::string id);
    crow::response handle_create_snapshot(const crow::request&, std::string id);
    crow::response handle_restore_snapshot(const crow::request&, std::string id);
    crow::response handle_list_snapshots(const crow::request&, std::string id);

    static json profile_to_json(const VMProfile& profile);
    static crow::response send_error(int status, const std::string& msg);
    static crow::response send_json(const json& j, int status = 200);
};
