set(BENCHMARK_SOURCES
    GeneratorBenchmark.cpp
    GeneratorBenchmark.h
    SingleThreadGenerator.cpp
    SingleThreadGenerator.h
    MultiThreadGenerator.cpp
    MultiThreadGenerator.h
    GpuPacketGenerator.cu
    GpuPacketGenerator.h
    LinuxSocketSender.cpp
    LinuxSocketSender.h
    TransmissionBenchmark.cpp
    TransmissionBenchmark.h
    HeavyPayloadGenerator.cpp
    HeavyPayloadGenerator.h
    IPacketGenerator.h
    CpuUtils.h
)

add_library(fuzzer_benchmark STATIC ${BENCHMARK_SOURCES})

target_include_directories(fuzzer_benchmark PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/fuzzing
    ${PROJECT_SOURCE_DIR}/src/fuzzing/strategies
    ${PROJECT_SOURCE_DIR}/src/utils
    ${DPDK_INCLUDE_DIRS}
)

target_link_libraries(fuzzer_benchmark PUBLIC 
    fuzzing_lib
    utils_lib
    ${DPDK_LIBRARIES}
    CUDA::cudart
)
