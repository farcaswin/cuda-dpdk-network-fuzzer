#include "ConfigManager.h"
#include "Logger.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

void ConfigManager::load(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    if (!fs::exists(config_path)) {
        LOG_WARN("Config file {} not found, using defaults", config_path);
        loaded_ = true;
        return;
    }

    try {
        std::ifstream f(config_path);
        config_data_ = nlohmann::json::parse(f);
        loaded_ = true;
        LOG_INFO("Configuration loaded from {}", config_path);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config file {}: {}", config_path, e.what());
        loaded_ = true; // Still mark as loaded to avoid repeated attempts
    }
}

std::string ConfigManager::get_vm_storage_dir() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (config_data_.contains("vm_storage_dir")) {
        return config_data_["vm_storage_dir"];
    }
    return DEFAULT_STORAGE_DIR;
}

std::string ConfigManager::get_vm_profiles_dir() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (config_data_.contains("vm_profiles_dir")) {
        return config_data_["vm_profiles_dir"];
    }
    return DEFAULT_PROFILES_DIR;
}

std::string ConfigManager::get_vm_templates_dir() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (config_data_.contains("vm_templates_dir")) {
        return config_data_["vm_templates_dir"];
    }
    return DEFAULT_TEMPLATES_DIR;
}

std::string ConfigManager::get_libvirt_network() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (config_data_.contains("libvirt_network")) {
        return config_data_["libvirt_network"];
    }
    return DEFAULT_NETWORK;
}

std::string ConfigManager::get_default_snapshot_prefix() const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (config_data_.contains("default_snapshot_prefix")) {
        return config_data_["default_snapshot_prefix"];
    }
    return DEFAULT_SNAPSHOT_PREFIX;
}
