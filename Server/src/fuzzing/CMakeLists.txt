add_library(fuzzing_lib STATIC
    kernels/IcmpFuzzStrategy.cu
    kernels/IpHeaderFuzzStrategy.cu
    kernels/TcpFuzzStrategy.cu
)

target_include_directories(fuzzing_lib PUBLIC 
    .
    strategies
    ${CUDAToolkit_INCLUDE_DIRS}
)

target_link_libraries(fuzzing_lib PRIVATE
    utils_lib
    CUDA::cudart
)
