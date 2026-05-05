add_library(domain_lib INTERFACE)

target_include_directories(domain_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(domain_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/NetworkInfo.h)
