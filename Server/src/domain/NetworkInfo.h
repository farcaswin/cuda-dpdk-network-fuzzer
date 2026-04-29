#pragma once
#include <string>

struct NetworkInfo{
    std::string interface_name;
    std::string local_ip;
    std::string subnet_mask;
    std::string subnet_cidr;
    std::string gateway;
    bool is_valid;
};