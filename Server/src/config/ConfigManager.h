#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <mutex>

class ConfigManager {
public:
    static ConfigManager& instance();

    void load(const std::string& config_path = "config.json");

    std::string get_vm_storage_dir() const;
    std::string get_vm_profiles_dir() const;
    std::string get_vm_templates_dir() const;
    std::string get_libvirt_network() const;
    std::string get_default_snapshot_prefix() const;

private:
    ConfigManager() = default;
    
    nlohmann::json config_data_;
    mutable std::mutex mtx_;
    bool loaded_ = false;

    // Default values
    const std::string DEFAULT_STORAGE_DIR = "/var/lib/libvirt/images/fuzzer_vms";
    const std::string DEFAULT_PROFILES_DIR = "vm-profiles";
    const std::string DEFAULT_TEMPLATES_DIR = "vm-profiles/templates";
    const std::string DEFAULT_NETWORK = "default";
    const std::string DEFAULT_SNAPSHOT_PREFIX = "pre-fuzz-";
};
