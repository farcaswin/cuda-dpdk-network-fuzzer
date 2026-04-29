#ifndef FUZZ_ROUTES_H
#define FUZZ_ROUTES_H

#include "RouteGroup.h"
#include "FuzzService.h"

namespace fuzzer {

class FuzzRoutes : public RouteGroup {
public:
    FuzzRoutes(FuzzService& service);
    void register_routes(httplib::Server& svr) override;

private:
    FuzzService& service_;
};

} // namespace fuzzer

#endif // FUZZ_ROUTES_H
