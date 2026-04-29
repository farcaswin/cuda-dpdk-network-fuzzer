#include "VMRoutes.h"
#include "Logger.h"
#include "ServiceException.h" 

VMRoutes::VMRoutes(VMService& vm_srv) : vm_service_(vm_srv) {}

void VMRoutes::register_routes(httplib::Server& srv) {
    srv.Get("/api/vm/profiles", [this](const auto& req, auto& res) { handle_get_profiles(req, res); });
    srv.Post("/api/vm/profiles", [this](const auto& req, auto& res) { handle_create_profile(req, res); });
    srv.Put("/api/vm/profiles/:id", [this](const auto& req, auto& res) { handle_update_profile(req, res); });
    srv.Get("/api/vm/storage/disks", [this](const auto& req, auto& res) { handle_list_available_disks(req, res); });
    
    srv.Post("/api/vm/:id/define", [this](const auto& req, auto& res) { handle_define_vm(req, res); });
    srv.Post("/api/vm/:id/start", [this](const auto& req, auto& res) { handle_start_vm(req, res); });
    srv.Post("/api/vm/:id/stop", [this](const auto& req, auto& res) { handle_stop_vm(req, res); });
    srv.Get("/api/vm/:id/status", [this](const auto& req, auto& res) { handle_get_status(req, res); });
    srv.Post("/api/vm/:id/snapshot", [this](const auto& req, auto& res) { handle_create_snapshot(req, res); });
    srv.Post("/api/vm/:id/restore", [this](const auto& req, auto& res) { handle_restore_snapshot(req, res); });
    srv.Get("/api/vm/:id/snapshots", [this](const auto& req, auto& res) { handle_list_snapshots(req, res); });
}

void VMRoutes::handle_get_profiles(const httplib::Request&, httplib::Response& res) {
    auto profiles = vm_service_.get_profiles();
    json j = json::array();
    for (const auto& p : profiles) {
        j.push_back(profile_to_json(p));
    }
    res.set_content(j.dump(4) + "\n", "application/json");
}

void VMRoutes::handle_create_profile(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);
        VMProfile p;
        p.id = j["id"];
        p.display_name = j["display_name"];
        if (j.contains("description")) p.description = j["description"];
        p.disk_image_name = j["disk_image_name"];
        p.xml_template = j["xml_template"];
        p.os_family = j["os_family"];
        p.recommended_strategies = j["recommended_strategies"].get<std::vector<std::string>>();
        if (j.contains("memory_mb")) p.memory_mb = j["memory_mb"];
        if (j.contains("vcpus")) p.vcpus = j["vcpus"];

        vm_service_.create_profile(p);
        res.set_content(json{{"status", "ok"}}.dump(4) + "\n", "application/json");
    } catch (const std::exception& e) {
        send_error(res, 400, e.what());
    }
}

void VMRoutes::handle_update_profile(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        auto j = json::parse(req.body);
        VMProfile p;
        p.id = id;
        p.display_name = j["display_name"];
        if (j.contains("description")) p.description = j["description"];
        p.disk_image_name = j["disk_image_name"];
        p.xml_template = j["xml_template"];
        p.os_family = j["os_family"];
        p.recommended_strategies = j["recommended_strategies"].get<std::vector<std::string>>();
        if (j.contains("memory_mb")) p.memory_mb = j["memory_mb"];
        if (j.contains("vcpus")) p.vcpus = j["vcpus"];

        vm_service_.update_profile(p);
        res.set_content(json{{"status", "ok"}}.dump(4) + "\n", "application/json");
    } catch (const std::exception& e) {
        send_error(res, 400, e.what());
    }
}

void VMRoutes::handle_list_available_disks(const httplib::Request&, httplib::Response& res) {
    auto disks = vm_service_.list_available_disks();
    res.set_content(json(disks).dump(4) + "\n", "application/json");
}

void VMRoutes::handle_define_vm(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        vm_service_.define_vm(id);
        res.set_content(json{{"status", "ok"}, {"message", "VM defined"}}.dump(4) + "\n", "application/json");
    } catch (const ServiceException& e) {
        send_error(res, e.error().http_status, e.error().message);
    } catch (const std::exception& e) {
        send_error(res, 500, e.what());
    }
}

void VMRoutes::handle_start_vm(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        vm_service_.start_vm(id);
        res.set_content(json{{"status", "ok"}}.dump(4) + "\n", "application/json");
    } catch (const std::exception& e) {
        send_error(res, 500, e.what());
    }
}

void VMRoutes::handle_stop_vm(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        vm_service_.stop_vm(id);
        res.set_content(json{{"status", "ok"}}.dump(4) + "\n", "application/json");
    } catch (const std::exception& e) {
        send_error(res, 500, e.what());
    }
}

void VMRoutes::handle_get_status(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        auto status = vm_service_.get_status(id);
        std::string ip = "";
        std::string mac = "";
        
        if (status.state == "running") {
            ip = vm_service_.get_vm_ip(id);
            mac = vm_service_.get_vm_mac(id);
        }

        res.set_content(json{
            {"name", status.name},
            {"state", status.state},
            {"exists", status.exists},
            {"ip", ip},
            {"mac", mac}
        }.dump(4) + "\n", "application/json");
    } catch (const std::exception& e) {
        send_error(res, 500, e.what());
    }
}

void VMRoutes::handle_create_snapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        std::string snap_name = "";
        if (!req.body.empty()) {
            auto j = json::parse(req.body);
            if (j.contains("name")) snap_name = j["name"];
        }
        std::string created_name = vm_service_.create_snapshot(id, snap_name);
        res.set_content(json{{"status", "ok"}, {"snapshot", created_name}}.dump(4) + "\n", "application/json");
    } catch (const std::exception& e) {
        send_error(res, 500, e.what());
    }
}

void VMRoutes::handle_restore_snapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        std::string snap_name = "";
        if (!req.body.empty()) {
            auto j = json::parse(req.body);
            if (j.contains("name")) snap_name = j["name"];
        }
        vm_service_.restore_snapshot(id, snap_name);
        res.set_content(json{{"status", "ok"}}.dump(4) + "\n", "application/json");
    } catch (const ServiceException& e) {
        send_error(res, e.error().http_status, e.error().message);
    } catch (const std::exception& e) {
        send_error(res, 500, e.what());
    }
}

void VMRoutes::handle_list_snapshots(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string id = req.path_params.at("id");
        auto snapshots = vm_service_.list_snapshots(id);
        res.set_content(json(snapshots).dump(4) + "\n", "application/json");
    } catch (const std::exception& e) {
        send_error(res, 500, e.what());
    }
}

json VMRoutes::profile_to_json(const VMProfile& profile) {
    return json{
        {"id", profile.id},
        {"display_name", profile.display_name},
        {"description", profile.description},
        {"disk_image_name", profile.disk_image_name},
        {"xml_template", profile.xml_template},
        {"os_family", profile.os_family},
        {"recommended_strategies", profile.recommended_strategies},
        {"memory_mb", profile.memory_mb},
        {"vcpus", profile.vcpus}
    };
}

void VMRoutes::send_error(httplib::Response& res, int status, const std::string& msg) {
    res.status = status;
    res.set_content(json{{"error", msg}}.dump(4) + "\n", "application/json");
}
