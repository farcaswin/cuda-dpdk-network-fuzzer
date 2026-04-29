#pragma once
#include "RouteGroup.h"
#include <httplib.h>
#include <vector>
#include <memory>
#include <string>

class HttpSrv{
public:
    HttpSrv(const std::string& host, int port);

    void add_route_group(std::unique_ptr<RouteGroup> group);

    void start();

    void stop();
private:
    std::string host_;
    int port_;

    httplib::Server server_;

    std::vector<std::unique_ptr<RouteGroup>> route_groups_;
    
    void setup_global_handlers();
};


