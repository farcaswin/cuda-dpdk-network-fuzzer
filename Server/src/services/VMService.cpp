#include "VMService.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "ServiceException.h" // For ServiceException
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <algorithm>

namespace fs = std::filesystem;

VMService::VMService() {
    profiles_dir_ = ConfigManager::instance().get_vm_profiles_dir();
    templates_dir_ = ConfigManager::instance().get_vm_templates_dir();
    load_profiles();
    setup_network();
}

VMService::~VMService() {
    cleanup();
}

void VMService::cleanup() {
    LOG_INFO("Cleaning up VMService resources...");
    std::lock_guard<std::mutex> lock(mtx_);
    
    for (const auto& [id, profile] : profiles_) {
        try {
            std::string cmd = "virsh domstate " + id + " 2>&1";
            std::string state = run_command(cmd);
            
            if (state.find("running") != std::string::npos) {
                LOG_INFO("Shutting down running VM: {}", id);
                run_command("virsh destroy " + id + " 2>/dev/null");
            }
        } catch (...) {
            // Ignore errors
        }
    }
}

void VMService::setup_network() {
    std::string net_name = ConfigManager::instance().get_libvirt_network();
    LOG_INFO("Checking libvirt network: {}", net_name);

    std::string check_cmd = "virsh net-info " + net_name + " 2>&1";
    std::string output = run_command(check_cmd);

    if (output.find("failed to get network") != std::string::npos || 
        output.find("error:") != std::string::npos) {
        
        LOG_INFO("Network {} not found, creating it...", net_name);
        
        // Define the network XML
        std::string net_xml = 
            "<network>\n"
            "  <name>" + net_name + "</name>\n"
            "  <bridge name='virbr1' stp='on' delay='0'/>\n"
            "  <ip address='10.10.10.1' netmask='255.255.255.0'>\n"
            "    <dhcp>\n"
            "      <range start='10.10.10.10' end='10.10.10.100'/>\n"
            "    </dhcp>\n"
            "  </ip>\n"
            "</network>";

        std::string temp_net_path = "/tmp/fuzzer_net.xml";
        std::ofstream out(temp_net_path);
        out << net_xml;
        out.close();

        run_command("virsh net-define " + temp_net_path);
        run_command("virsh net-start " + net_name);
        run_command("virsh net-autostart " + net_name);
        
        fs::remove(temp_net_path);

        // Validation: Check if it actually exists now
        std::string verify_cmd = "virsh net-info " + net_name + " 2>&1";
        std::string verify_out = run_command(verify_cmd);
        if (verify_out.find("Active:         yes") == std::string::npos) {
            LOG_CRITICAL("CRITICAL: Failed to create or start network {}. Output: {}", net_name, verify_out);
            throw std::runtime_error("Network creation failed: " + net_name);
        }

        LOG_INFO("Network {} created and started successfully", net_name);
    } else {
        LOG_INFO("Network {} is already configured", net_name);
        
        // Ensure it's active
        if (output.find("Active:         no") != std::string::npos) {
            LOG_INFO("Network {} is inactive, starting it...", net_name);
            run_command("virsh net-start " + net_name);
            
            // Re-verify after start attempt
            std::string verify_out = run_command("virsh net-info " + net_name + " 2>&1");
            if (verify_out.find("Active:         yes") == std::string::npos) {
                LOG_CRITICAL("CRITICAL: Failed to start existing network {}. Output: {}", net_name, verify_out);
                throw std::runtime_error("Failed to start network: " + net_name);
            }
        }
    }
}

void VMService::load_profiles() {
    std::lock_guard<std::mutex> lock(mtx_);
    profiles_.clear();

    if (!fs::exists(profiles_dir_)) {
        LOG_WARN("Profiles directory {} does not exist", profiles_dir_);
        return;
    }

    for (const auto& entry : fs::directory_iterator(profiles_dir_)) {
        if (entry.path().extension() == ".json") {
            try {
                std::ifstream f(entry.path());
                nlohmann::json data = nlohmann::json::parse(f);

                VMProfile profile;
                profile.id = data["id"];
                profile.display_name = data["display_name"];
                if (data.contains("description")) profile.description = data["description"];
                profile.disk_image_name = data["disk_image_name"];
                profile.xml_template = data["xml_template"];
                profile.os_family = data["os_family"];
                if (data.contains("target_ip")) profile.target_ip = data["target_ip"];
                if (data.contains("target_mac")) profile.target_mac = data["target_mac"];
                profile.recommended_strategies = data["recommended_strategies"].get<std::vector<std::string>>();
                if (data.contains("memory_mb")) profile.memory_mb = data["memory_mb"];
                if (data.contains("vcpus")) profile.vcpus = data["vcpus"];

                profiles_[profile.id] = profile;
                LOG_INFO("Loaded VM profile: {} ({})", profile.display_name, profile.id);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to load profile {}: {}", entry.path().string(), e.what());
            }
        }
    }
}

std::vector<VMProfile> VMService::get_profiles() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<VMProfile> result;
    for (const auto& [id, profile] : profiles_) {
        result.push_back(profile);
    }
    return result;
}

std::optional<VMProfile> VMService::get_profile(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = profiles_.find(id);
    if (it != profiles_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void VMService::update_profile(const VMProfile& profile) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        profiles_[profile.id] = profile;
    }
    save_profile_to_disk(profile);
    LOG_INFO("Updated VM profile: {}", profile.id);
}

void VMService::create_profile(const VMProfile& profile) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (profiles_.find(profile.id) != profiles_.end()) {
            throw ServiceException({400, "Profile ID already exists"});
        }
        profiles_[profile.id] = profile;
    }
    save_profile_to_disk(profile);
    LOG_INFO("Created new VM profile: {}", profile.id);
}

std::vector<std::string> VMService::list_available_disks() const {
    std::string storage_dir = ConfigManager::instance().get_vm_storage_dir();
    std::vector<std::string> disks;
    
    if (!fs::exists(storage_dir)) {
        LOG_WARN("Storage directory {} does not exist", storage_dir);
        return disks;
    }

    for (const auto& entry : fs::directory_iterator(storage_dir)) {
        std::string ext = entry.path().extension().string();
        if (ext == ".qcow2" || ext == ".img" || ext == ".raw" || ext == ".iso") {
            disks.push_back(entry.path().filename().string());
        }
    }
    return disks;
}

void VMService::save_profile_to_disk(const VMProfile& profile) {
    nlohmann::json data;
    data["id"] = profile.id;
    data["display_name"] = profile.display_name;
    data["description"] = profile.description;
    data["disk_image_name"] = profile.disk_image_name;
    data["xml_template"] = profile.xml_template;
    data["os_family"] = profile.os_family;
    data["target_ip"] = profile.target_ip;
    data["target_mac"] = profile.target_mac;
    data["recommended_strategies"] = profile.recommended_strategies;
    data["memory_mb"] = profile.memory_mb;
    data["vcpus"] = profile.vcpus;

    std::string file_path = profiles_dir_ + "/" + profile.id + ".json";
    std::ofstream f(file_path);
    f << data.dump(4);
}

void VMService::define_vm(const std::string& profile_id) {
    auto profile_opt = get_profile(profile_id);
    if (!profile_opt) {
        throw ServiceException({404, "Profile not found"});
    }

    // Try to undefine first to ensure we update the configuration correctly
    run_command("virsh undefine " + profile_id + " --snapshots-metadata 2>/dev/null");

    std::string xml = generate_xml(*profile_opt);
    std::string temp_xml_path = "/tmp/" + profile_id + ".xml";
    
    std::ofstream out(temp_xml_path);
    out << xml;
    out.close();

    std::string cmd = "virsh define " + temp_xml_path + " 2>&1";
    std::string output = run_command(cmd);
    
    if (output.find("error:") != std::string::npos) {
        LOG_ERROR("Failed to define VM {}: {}", profile_id, output);
        fs::remove(temp_xml_path);
        throw ServiceException({500, "Define failed: " + output});
    }

    LOG_INFO("Defined VM {}: {}", profile_id, output);
    fs::remove(temp_xml_path);

    // Register static IP in libvirt network if provided
    if (!profile_opt->target_ip.empty() && !profile_opt->target_mac.empty()) {
        std::string net_name = ConfigManager::instance().get_libvirt_network();
        LOG_INFO("Registering static IP {} for MAC {} in network {}", 
                 profile_opt->target_ip, profile_opt->target_mac, net_name);
        
        // Try to add it (it might already exist, so we use 'add' then 'modify' if needed, 
        // but 'add' will fail if exists. libvirt net-update is a bit tricky)
        // A simpler way is to try to delete it first (ignore error) then add it.
        std::string del_cmd = "virsh net-update " + net_name + " delete ip-dhcp-host " +
                             "\"<host mac='" + profile_opt->target_mac + "'/>\" --live --config 2>/dev/null";
        run_command(del_cmd);

        std::string add_cmd = "virsh net-update " + net_name + " add ip-dhcp-host " +
                             "\"<host mac='" + profile_opt->target_mac + "' ip='" + profile_opt->target_ip + "'/>\" --live --config";
        run_command(add_cmd);
    }
}

void VMService::start_vm(const std::string& name) {
    std::string cmd = "virsh start " + name + " 2>&1";
    std::string output = run_command(cmd);
    if (output.find("error:") != std::string::npos) {
        LOG_ERROR("Failed to start VM {}: {}", name, output);
        throw ServiceException({500, "Start failed: " + output});
    }
    LOG_INFO("Started VM {}", name);
}

void VMService::stop_vm(const std::string& name) {
    std::string cmd = "virsh destroy " + name + " 2>&1";
    std::string output = run_command(cmd);
    if (output.find("error:") != std::string::npos && output.find("domain is not running") == std::string::npos) {
        LOG_ERROR("Failed to stop VM {}: {}", name, output);
        throw ServiceException({500, "Stop failed: " + output});
    }
    LOG_INFO("Stopped VM {}", name);
}

VMStatus VMService::get_status(const std::string& name) {
    std::string cmd = "virsh domstate " + name + " 2>&1";
    std::string output = run_command(cmd);
    
    VMStatus status;
    status.name = name;
    
    if (output.find("failed to get domain") != std::string::npos || 
        output.find("error:") != std::string::npos) {
        status.exists = false;
        status.state = "unknown";
    } else {
        status.exists = true;
        // Clean up output (remove trailing newline)
        output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
        status.state = output;
    }
    
    return status;
}

std::string VMService::create_snapshot(const std::string& vm_name, const std::string& snapshot_name) {
    std::string snap = snapshot_name;
    if (snap.empty()) {
        snap = vm_name + "-clean-state";
    }

    std::string cmd = "virsh snapshot-create-as " + vm_name + " " + snap + " 2>&1";
    std::string output = run_command(cmd);
    
    if (output.find("error:") != std::string::npos) {
        LOG_ERROR("Failed to create snapshot for VM {}: {}", vm_name, output);
        throw ServiceException({500, "Snapshot creation failed: " + output});
    }

    LOG_INFO("Created snapshot {} for VM {}", snap, vm_name);
    return snap;
}

void VMService::restore_snapshot(const std::string& vm_name, const std::string& snapshot_name) {
    std::string snap = snapshot_name;
    if (snap.empty()) {
        auto snapshots = list_snapshots(vm_name);
        if (snapshots.empty()) {
            throw ServiceException({404, "No snapshots found to restore for VM: " + vm_name});
        }
        // Use the most recent one (last in the list)
        snap = snapshots.back();
        LOG_INFO("No snapshot name provided, using latest: {}", snap);
    }

    std::string cmd = "virsh snapshot-revert " + vm_name + " " + snap + " 2>&1";
    std::string output = run_command(cmd);
    
    if (output.find("error:") != std::string::npos) {
        LOG_ERROR("Failed to restore snapshot for VM {}: {}", vm_name, output);
        throw ServiceException({500, "Snapshot restore failed: " + output});
    }

    LOG_INFO("Restored snapshot {} for VM {}", snap, vm_name);
}

std::vector<std::string> VMService::list_snapshots(const std::string& vm_name) {
    std::string cmd = "virsh snapshot-list " + vm_name + " --name";
    std::string output = run_command(cmd);
    
    std::vector<std::string> snapshots;
    std::stringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        if (!line.empty()) {
            snapshots.push_back(line);
        }
    }
    return snapshots;
}

std::string VMService::get_vm_ip(const std::string& name) {
    auto profile_opt = get_profile(name);
    if (profile_opt && !profile_opt->target_ip.empty()) {
        return profile_opt->target_ip;
    }

    // Fallback to old method if no static IP is defined
    std::string net_name = ConfigManager::instance().get_libvirt_network();
    std::string mac = get_vm_mac(name);
    if (mac.empty()) return "";

    std::string cmd = "virsh net-dhcp-leases " + net_name + " --mac " + mac + " | awk 'NR==3 {print $5}' | cut -d'/' -f1";
    std::string ip = run_command(cmd);
    ip.erase(std::remove(ip.begin(), ip.end(), '\n'), ip.end());
    ip.erase(std::remove(ip.begin(), ip.end(), '\r'), ip.end());
    
    return ip;
}

std::string VMService::get_vm_mac(const std::string& name) {
    auto profile_opt = get_profile(name);
    if (profile_opt && !profile_opt->target_mac.empty()) {
        return profile_opt->target_mac;
    }

    // Fallback to old method
    std::string net_name = ConfigManager::instance().get_libvirt_network();
    std::string cmd = "virsh domiflist " + name + " | awk '/" + net_name + "/ {print $5}'";
    std::string mac = run_command(cmd);
    mac.erase(std::remove(mac.begin(), mac.end(), '\n'), mac.end());
    mac.erase(std::remove(mac.begin(), mac.end(), '\r'), mac.end());
    
    return mac;
}

std::string VMService::run_command(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    int status = pclose(pipe);
    if (status != 0 && result.find("error:") != std::string::npos) {
        LOG_DEBUG("Command '{}' exited with status {}. Output: {}", cmd, status, result);
    }
    
    return result;
}

std::string VMService::generate_xml(const VMProfile& profile) {
    std::string template_path = templates_dir_ + "/" + profile.xml_template;
    if (!fs::exists(template_path)) {
        // Try fallback to profiles_dir if templates_dir fails
        template_path = profiles_dir_ + "/" + profile.xml_template;
        if (!fs::exists(template_path)) {
            throw ServiceException({500, "XML template not found: " + profile.xml_template});
        }
    }

    std::ifstream f(template_path);
    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();

    std::string storage_dir = ConfigManager::instance().get_vm_storage_dir();
    if (storage_dir.back() != '/') storage_dir += '/';
    
    std::string full_disk_path = storage_dir + profile.disk_image_name;

    // Helper lambda for multiple replacements
    auto replace_all = [&](std::string& str, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replace_all(content, "{{DISK_PATH}}", full_disk_path);
    replace_all(content, "{{MEMORY_KB}}", std::to_string(profile.memory_mb * 1024));
    replace_all(content, "{{VCPUS}}", std::to_string(profile.vcpus));
    replace_all(content, "{{VM_NAME}}", profile.id);
    replace_all(content, "{{TARGET_MAC}}", profile.target_mac);

    return content;
}
