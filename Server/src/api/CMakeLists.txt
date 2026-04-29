add_library(api_lib STATIC 
    HttpSrv.cpp 
    HttpSrv.h
    RouteGroup.h
    routing/VMRoutes.h
    routing/VMRoutes.cpp
    routing/FuzzRoutes.h
    routing/FuzzRoutes.cpp
)

target_include_directories(api_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(api_lib PUBLIC 
    httplib
    nlohmann_json
    service_lib
    utils_lib
)
