add_library(domain_lib STATIC 
    NetworkInfo.h
)

target_include_directories(domain_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
