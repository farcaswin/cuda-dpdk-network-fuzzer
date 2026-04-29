add_library(utils_lib Logger.h Logger.cpp)

target_include_directories(utils_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(utils_lib PUBLIC
    spdlog::spdlog
)
