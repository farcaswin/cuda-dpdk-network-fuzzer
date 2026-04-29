add_library(service_lib STATIC 
    VMService.cpp
    VMService.h
    FuzzService.cpp
    FuzzService.h
)
target_include_directories(service_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(service_lib PUBLIC 
    domain_lib
    utils_lib
    config_lib
    fuzzing_lib
)
