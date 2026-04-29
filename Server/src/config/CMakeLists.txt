add_library(config_lib STATIC 
    ConfigManager.cpp 
    ConfigManager.h
)

target_include_directories(config_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(config_lib PUBLIC 
    nlohmann_json
    utils_lib
)
