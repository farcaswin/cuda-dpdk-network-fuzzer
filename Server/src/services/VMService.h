#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <optional>

struct VMProfile {
    std::string id;
    std::string display_name;
    std::string description;
    std::string disk_image_name;
    std::string xml_template;
    std::string os_family;
    std::string target_ip;
    std::string target_mac;
    std::vector<std::string> recommended_strategies;
    int memory_mb = 1024;
    int vcpus = 1;
};

struct VMStatus {
    std::string name;
    std::string state; // running, shut off, paused
    bool exists;
};

class VMService {
public:
    VMService();
    ~VMService();

    void cleanup();

    void load_profiles();
    std::vector<VMProfile> get_profiles() const;
    std::optional<VMProfile> get_profile(const std::string& id) const;

    // Profile Management
    void update_profile(const VMProfile& profile);
    void create_profile(const VMProfile& profile);
    std::vector<std::string> list_available_disks() const;

    // VM lifecycle
    void define_vm(const std::string& profile_id);
    void start_vm(const std::string& name);
    void stop_vm(const std::string& name);
    VMStatus get_status(const std::string& name);

    // Snapshots
    std::string create_snapshot(const std::string& vm_name, const std::string& snapshot_name = "");
    void restore_snapshot(const std::string& vm_name, const std::string& snapshot_name = "");
    std::vector<std::string> list_snapshots(const std::string& vm_name);

    // Network Info
    std::string get_vm_ip(const std::string& name);
    std::string get_vm_mac(const std::string& name);

    void setup_network();

private:
    std::string profiles_dir_;
    std::string templates_dir_;
    std::map<std::string, VMProfile> profiles_;
    mutable std::mutex mtx_;

    std::string run_command(const std::string& cmd);
    std::string generate_xml(const VMProfile& profile);
    void save_profile_to_disk(const VMProfile& profile);
};
