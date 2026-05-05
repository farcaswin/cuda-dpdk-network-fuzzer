#ifndef FUZZ_SERVICE_H
#define FUZZ_SERVICE_H

#include "FuzzEngine.h"
#include "DpdkSender.h"
#include "VMService.h"
#include "TargetMonitor.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <map>

namespace fuzzer {

struct FuzzParams {
    std::string vm_id;
    std::string target_ip;
    std::string target_mac;
    std::string src_ip;
    std::string src_mac;
    std::string strategy;
    uint32_t batch_size = 65536;
    bool auto_restore = false;
};

class FuzzService {
public:
    FuzzService(DpdkSender& dpdk_sender, VMService& vm_service);
    ~FuzzService() = default;

    void start_fuzzing(const FuzzParams& params);
    void stop_fuzzing();
    
    bool is_fuzzing() const;
    nlohmann::json get_status() const;

private:
    uint32_t ip_to_uint(const std::string& ip) const;
    void mac_to_array(const std::string& mac, uint8_t* out) const;

    DpdkSender& dpdk_sender_;
    VMService& vm_service_;
    FuzzEngine engine_;
    TargetMonitor monitor_;
};

} // namespace fuzzer

#endif // FUZZ_SERVICE_H
