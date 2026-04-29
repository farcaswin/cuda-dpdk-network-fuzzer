#pragma once
#include <httplib.h>

class RouteGroup{
public:
    virtual ~RouteGroup() = default;

    virtual void register_routes(httplib::Server& svr) = 0;
};