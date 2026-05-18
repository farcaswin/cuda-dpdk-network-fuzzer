#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include "GeneratorBenchmark.h"
#include "TransmissionBenchmark.h"
#include "HeavyPayloadGenerator.h"
#include "DpdkSender.h"
#include "Logger.h"

using namespace fuzzer;

void print_usage() {
    std::cout << "FuzzerServer Benchmark Suite\n"
              << "Usage: fuzzer-bench [options]\n"
              << "Options:\n"
              << "  --mode <gen|linux|dpdk|all>  Select benchmark section (default: all)\n"
              << "  --iface <name>               Linux interface for socket tests (default: lo)\n"
              << "  --vdev <dpdk_args>           Custom DPDK vdev (default: net_tap0,iface=dpdk0)\n"
              << "  --help                       Show this help message\n";
}

int main(int argc, char** argv) {
    Logger::init();
    
    std::string mode = "all";
    std::string linux_iface = "lo";
    std::string vdev_arg = "net_tap0,iface=dpdk0";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) mode = argv[++i];
        else if (arg == "--iface" && i + 1 < argc) linux_iface = argv[++i];
        else if (arg == "--vdev" && i + 1 < argc) vdev_arg = argv[++i];
        else if (arg == "--help") { print_usage(); return 0; }
    }

    LOG_INFO("Starting Benchmark: mode={}, iface={}, vdev={}", mode, linux_iface, vdev_arg);

    PacketGenConfig config;
    config.src_mac[0] = 0x00; config.src_mac[1] = 0x11; config.src_mac[2] = 0x22;
    config.src_mac[3] = 0x33; config.src_mac[4] = 0x44; config.src_mac[5] = 0x55;
    config.dest_mac[0] = 0xAA; config.dest_mac[1] = 0xBB; config.dest_mac[2] = 0xCC;
    config.dest_mac[3] = 0xDD; config.dest_mac[4] = 0xEE; config.dest_mac[5] = 0xFF;
    config.src_ip = 0x01020304;
    config.dest_ip = 0x05060708;

    // SECTION 1: Generation
    if (mode == "all" || mode == "gen") {
        LOG_INFO("=== Section 1: Strict Packet Generation ===");
        std::vector<uint32_t> gen_batches = {1024, 4096, 16384, 65536, 131072, 262144};
        GeneratorBenchmark gen_bench(gen_batches, 10);
        auto gen_report = gen_bench.run_all(config);
        
        std::cout << gen_report.to_table() << std::endl;
        
        std::string path = "scripts/results/benchmark_generation.csv";
        std::ofstream csv(path);
        if (csv.is_open()) {
            csv << gen_report.to_csv();
            csv.flush();
            csv.close();
            LOG_INFO("Generation results saved to {}", path);
        } else {
            LOG_ERROR("Failed to open {} for writing! Ensure directory 'scripts/results/' exists.", path);
        }
        cudaDeviceReset();
    }

    // SECTION 2: Linux Sockets (Independent of DPDK)
    if (mode == "all" || mode == "linux") {
        LOG_INFO("=== Section 2: Linux Sockets Pipeline ===");
        DpdkSender dummy_dpdk; // Not used for linux mode but needed for constructor
        TransmissionBenchmark trans_bench(dummy_dpdk, linux_iface);
        
        PipelineReport report;
        SingleThreadGenerator st_gen;
        std::vector<uint32_t> batches = {1024, 4096, 16384, 65536};
        
        for (uint32_t b : batches) {
            report.runs.push_back(trans_bench.run_linux_sockets(st_gen, b, config));
        }

        std::cout << report.to_table() << std::endl;
        std::string path = "scripts/results/benchmark_linux.csv";
        std::ofstream csv(path);
        if (csv.is_open()) {
            csv << report.to_csv();
            csv.flush();
            csv.close();
            LOG_INFO("Linux results saved to {}", path);
        } else {
            LOG_ERROR("Failed to open {} for writing! Ensure directory 'scripts/results/' exists.", path);
        }
    }

    // SECTION 3: DPDK Pipeline
    if (mode == "all" || mode == "dpdk") {
        LOG_INFO("=== Section 3: DPDK Pipeline ===");
        DpdkSender dpdk_sender;
        
        char* eal_argv[] = { (char*)"fuzzer_bench", (char*)"--vdev", (char*)vdev_arg.c_str(), (char*)"--no-pci", (char*)"-l", (char*)"0" };
        int eal_argc = 6;
        
        if (dpdk_sender.init(eal_argc, eal_argv)) {
            TransmissionBenchmark trans_bench(dpdk_sender, linux_iface);
            
            PipelineReport report;
            SingleThreadGenerator st_gen;
            MultiThreadGenerator mt_gen;
            HeavyPayloadGenerator heavy_cpu_gen;
            
            std::vector<uint32_t> batches = {1024, 4096, 16384, 65536};
            for (uint32_t b : batches) {
                // 1. ICMP Comparison (Simple)
                report.runs.push_back(trans_bench.run_dpdk(st_gen, b, config));
                report.runs.push_back(trans_bench.run_dpdk(mt_gen, b, config));
                report.runs.push_back(trans_bench.run_fuzz_engine_icmp(b, config, 2));

                // 2. Heavy Payload Comparison (Compute Bound)
                report.runs.push_back(trans_bench.run_dpdk(heavy_cpu_gen, b, config));
                report.runs.push_back(trans_bench.run_fuzz_engine(b, config, 2));
            }

            std::cout << report.to_table() << std::endl;
            std::string path = "scripts/results/benchmark_dpdk.csv";
            std::ofstream csv(path);
            if (csv.is_open()) {
                csv << report.to_csv();
                csv.flush();
                csv.close();
                LOG_INFO("DPDK results saved to {}", path);
            } else {
                LOG_ERROR("Failed to open {} for writing! Ensure directory 'scripts/results/' exists.", path);
            }
            dpdk_sender.shutdown();
        }
    }

    LOG_INFO("Benchmark Suite completed.");
    return 0;
}
