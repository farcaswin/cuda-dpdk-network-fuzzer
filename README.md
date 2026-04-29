# My Cool Fuzzer Project

So, this is my thesis project. The main idea is that I'm trying to make a network fuzzer that's actually fast. 

Normal fuzzers use the CPU and they are kinda slow because the Linux kernel gets in the way. I'm using **CUDA** (the GPU) to generate like a million packets at once, and **DPDK** to send them directly to the network card, skipping the slow parts of the OS.

I'm targeting **Virtual Machines** (KVM) because if I crash something, I can just hit 'reset' on a snapshot and try again without breaking my whole lab.

### What's inside:
- **Server**: The C++ part that does the heavy lifting with the GPU.
- **Client**: A Python app so I don't have to use the terminal for everything.

I hope this actually works and I get my degree lol.
