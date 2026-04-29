cmake_minimum_required(VERSION 3.20)
project(FuzzerServer CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find CUDA Toolkit
find_package(CUDAToolkit REQUIRED)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(spdlog)

add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/httplib
)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/json
)

add_subdirectory(src/domain)
add_subdirectory(src/config)
add_subdirectory(src/services)
add_subdirectory(src/api)
add_subdirectory(src/utils)
add_subdirectory(src/fuzzing)

add_executable(fuzzer-srv main.cpp)

target_link_libraries(fuzzer-srv PRIVATE 
    utils_lib
    api_lib
    service_lib
    domain_lib
    config_lib
    fuzzing_lib
    spdlog::spdlog_header_only
    CUDA::cudart
)
