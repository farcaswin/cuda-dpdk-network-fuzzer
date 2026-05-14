#pragma once
#include <crow.h>

class RouteGroup{
public:
    virtual ~RouteGroup() = default;

    virtual void register_routes(crow::SimpleApp& app) = 0;
};