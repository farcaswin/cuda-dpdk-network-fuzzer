find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_library(fuzzing_lib STATIC
    DpdkSender.cpp
    PacketBuffer.cpp
    FuzzEngine.cpp
    kernels/IcmpFuzzStrategy.cu
    kernels/IpHeaderFuzzStrategy.cu
    kernels/TcpFuzzStrategy.cu
)

# ISA Optimization for high-performance DPDK operations (rte_memcpy)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)

if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "Fuzzing Engine: Using -march=native for maximum CPU performance")
    target_compile_options(fuzzing_lib PRIVATE -march=native)
else()
    target_compile_options(fuzzing_lib PRIVATE ${DPDK_CFLAGS})
endif()

target_include_directories(fuzzing_lib PUBLIC 
    .
    strategies
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
    ${DPDK_INCLUDE_DIRS}
    ${CUDAToolkit_INCLUDE_DIRS}
)

target_link_libraries(fuzzing_lib PRIVATE
    ${DPDK_LIBRARIES}
    utils_lib
    CUDA::cudart
)
