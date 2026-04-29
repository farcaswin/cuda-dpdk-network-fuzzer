#pragma once
#include "VMService.h"
#include "RouteGroup.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class VMRoutes : public RouteGroup {
public:
    explicit VMRoutes(VMService& vm_srv);
    void register_routes(httplib::Server& srv) override;

private:
    VMService& vm_service_;

    // Handlers
    void handle_get_profiles(const httplib::Request&, httplib::Response&);
    void handle_create_profile(const httplib::Request&, httplib::Response&);
    void handle_update_profile(const httplib::Request&, httplib::Response&);
    void handle_list_available_disks(const httplib::Request&, httplib::Response&);
    void handle_define_vm(const httplib::Request&, httplib::Response&);
    void handle_start_vm(const httplib::Request&, httplib::Response&);
    void handle_stop_vm(const httplib::Request&, httplib::Response&);
    void handle_get_status(const httplib::Request&, httplib::Response&);
    void handle_create_snapshot(const httplib::Request&, httplib::Response&);
    void handle_restore_snapshot(const httplib::Request&, httplib::Response&);
    void handle_list_snapshots(const httplib::Request&, httplib::Response&);

    static json profile_to_json(const VMProfile& profile);
    static void send_error(httplib::Response& res, int status, const std::string& msg);
};
