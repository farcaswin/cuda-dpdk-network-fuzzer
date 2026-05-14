#include "VMRoutes.h"
#include "Logger.h"
#include "ServiceException.h" 

VMRoutes::VMRoutes(VMService& vm_srv) : vm_service_(vm_srv) {}

void VMRoutes::register_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/vm/profiles").methods(crow::HTTPMethod::GET)
    ([this](const crow::request& req) { return handle_get_profiles(req); });

    CROW_ROUTE(app, "/api/vm/profiles").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) { return handle_create_profile(req); });

    CROW_ROUTE(app, "/api/vm/profiles/<string>").methods(crow::HTTPMethod::PUT)
    ([this](const crow::request& req, std::string id) { return handle_update_profile(req, id); });

    CROW_ROUTE(app, "/api/vm/storage/disks").methods(crow::HTTPMethod::GET)
    ([this](const crow::request& req) { return handle_list_available_disks(req); });
    
    CROW_ROUTE(app, "/api/vm/<string>/define").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req, std::string id) { return handle_define_vm(req, id); });

    CROW_ROUTE(app, "/api/vm/<string>/start").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req, std::string id) { return handle_start_vm(req, id); });

    CROW_ROUTE(app, "/api/vm/<string>/stop").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req, std::string id) { return handle_stop_vm(req, id); });

    CROW_ROUTE(app, "/api/vm/<string>/status").methods(crow::HTTPMethod::GET)
    ([this](const crow::request& req, std::string id) { return handle_get_status(req, id); });

    CROW_ROUTE(app, "/api/vm/<string>/snapshot").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req, std::string id) { return handle_create_snapshot(req, id); });

    CROW_ROUTE(app, "/api/vm/<string>/restore").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req, std::string id) { return handle_restore_snapshot(req, id); });

    CROW_ROUTE(app, "/api/vm/<string>/snapshots").methods(crow::HTTPMethod::GET)
    ([this](const crow::request& req, std::string id) { return handle_list_snapshots(req, id); });
}

crow::response VMRoutes::handle_get_profiles(const crow::request&) {
    auto profiles = vm_service_.get_profiles();
    json j = json::array();
    for (const auto& p : profiles) {
        j.push_back(profile_to_json(p));
    }
    return send_json(j);
}

crow::response VMRoutes::handle_create_profile(const crow::request& req) {
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
        return send_json(json{{"status", "ok"}});
    } catch (const std::exception& e) {
        return send_error(400, e.what());
    }
}

crow::response VMRoutes::handle_update_profile(const crow::request& req, std::string id) {
    try {
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
        return send_json(json{{"status", "ok"}});
    } catch (const std::exception& e) {
        return send_error(400, e.what());
    }
}

crow::response VMRoutes::handle_list_available_disks(const crow::request&) {
    auto disks = vm_service_.list_available_disks();
    return send_json(json(disks));
}

crow::response VMRoutes::handle_define_vm(const crow::request& req, std::string id) {
    try {
        vm_service_.define_vm(id);
        return send_json(json{{"status", "ok"}, {"message", "VM defined"}});
    } catch (const ServiceException& e) {
        return send_error(e.error().http_status, e.error().message);
    } catch (const std::exception& e) {
        return send_error(500, e.what());
    }
}

crow::response VMRoutes::handle_start_vm(const crow::request& req, std::string id) {
    try {
        vm_service_.start_vm(id);
        return send_json(json{{"status", "ok"}});
    } catch (const std::exception& e) {
        return send_error(500, e.what());
    }
}

crow::response VMRoutes::handle_stop_vm(const crow::request& req, std::string id) {
    try {
        vm_service_.stop_vm(id);
        return send_json(json{{"status", "ok"}});
    } catch (const std::exception& e) {
        return send_error(500, e.what());
    }
}

crow::response VMRoutes::handle_get_status(const crow::request& req, std::string id) {
    try {
        auto status = vm_service_.get_status(id);
        std::string ip = "";
        std::string mac = "";
        
        if (status.state == "running") {
            ip = vm_service_.get_vm_ip(id);
            mac = vm_service_.get_vm_mac(id);
        }

        return send_json(json{
            {"name", status.name},
            {"state", status.state},
            {"exists", status.exists},
            {"ip", ip},
            {"mac", mac}
        });
    } catch (const std::exception& e) {
        return send_error(500, e.what());
    }
}

crow::response VMRoutes::handle_create_snapshot(const crow::request& req, std::string id) {
    try {
        std::string snap_name = "";
        if (!req.body.empty()) {
            auto j = json::parse(req.body);
            if (j.contains("name")) snap_name = j["name"];
        }
        std::string created_name = vm_service_.create_snapshot(id, snap_name);
        return send_json(json{{"status", "ok"}, {"snapshot", created_name}});
    } catch (const std::exception& e) {
        return send_error(500, e.what());
    }
}

crow::response VMRoutes::handle_restore_snapshot(const crow::request& req, std::string id) {
    try {
        std::string snap_name = "";
        if (!req.body.empty()) {
            auto j = json::parse(req.body);
            if (j.contains("name")) snap_name = j["name"];
        }
        vm_service_.restore_snapshot(id, snap_name);
        return send_json(json{{"status", "ok"}});
    } catch (const ServiceException& e) {
        return send_error(e.error().http_status, e.error().message);
    } catch (const std::exception& e) {
        return send_error(500, e.what());
    }
}

crow::response VMRoutes::handle_list_snapshots(const crow::request& req, std::string id) {
    try {
        auto snapshots = vm_service_.list_snapshots(id);
        return send_json(json(snapshots));
    } catch (const std::exception& e) {
        return send_error(500, e.what());
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

crow::response VMRoutes::send_error(int status, const std::string& msg) {
    crow::response res(json{{"error", msg}}.dump(4) + "\n");
    res.code = status;
    res.set_header("Content-Type", "application/json");
    res.add_header("Access-Control-Allow-Origin", "*");
    return res;
}

crow::response VMRoutes::send_json(const json& j, int status) {
    crow::response res(j.dump(4) + "\n");
    res.code = status;
    res.set_header("Content-Type", "application/json");
    res.add_header("Access-Control-Allow-Origin", "*");
    return res;
}
