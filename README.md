# CUDA DPDK Fuzzer Project

Thesis project. Network fuzzer that is very fast. 

Normal fuzzers use the CPU and they are slow because the Linux kernel gets in the way. I'm using **CUDA** (the GPU) to generate millions of network packets at once, and **DPDK** to send them directly to the NIC, skipping the slow parts of the OS.

I'm targeting **Virtual Machines** (KVM), so the whole fuzzing environment is virtual.

### What's inside:
- **Server**: C++ implementation with CUDA and DPDK.
- **Client**: A Python app using PyQt6.

