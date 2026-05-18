import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# Set global style for academic charts
plt.style.use('seaborn-v0_8-muted')
plt.rcParams.update({'font.size': 11, 'axes.grid': True, 'grid.alpha': 0.3})

def plot_generation():
    # Load from the correct location (results/)
    csv_path = 'results/benchmark_generation.csv'
    if not os.path.exists(csv_path):
        # Try local if script run from scripts/
        csv_path = 'benchmark_generation.csv'
        if not os.path.exists(csv_path):
            print(f"Error: {csv_path} not found!")
            return

    df = pd.read_csv(csv_path)
    
    # Filtering and sorting
    batches = sorted(df['batch_size'].unique())
    generators = df['generator'].unique()

    plt.figure(figsize=(10, 6))
    
    for gen in generators:
        gen_data = df[df['generator'] == gen].sort_values('batch_size')
        plt.plot(gen_data['batch_size'], gen_data['mpps'], marker='o', linewidth=2, label=gen)

    plt.xscale('log', base=2)
    plt.xlabel('Batch Size (Packets)')
    plt.ylabel('Throughput (Mpps)')
    plt.title('Strict Packet Generation Performance (Compute Bound)')
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.5)
    
    plt.tight_layout()
    plt.savefig('results/final_generation_performance.png', dpi=300)
    plt.savefig('results/final_generation_performance.pdf')
    print("Saved aggregated generation plots to results/")

def plot_pipeline():
    # Files to aggregate
    linux_csv = 'results/benchmark_linux.csv'
    dpdk_csv = 'results/benchmark_dpdk.csv'
    
    dataframes = []
    if os.path.exists(linux_csv):
        dataframes.append(pd.read_csv(linux_csv))
    else:
        print(f"Warning: {linux_csv} missing.")
        
    if os.path.exists(dpdk_csv):
        dataframes.append(pd.read_csv(dpdk_csv))
    else:
        print(f"Warning: {dpdk_csv} missing.")

    if not dataframes:
        print("Error: No pipeline CSV files found to aggregate.")
        return

    # Combine all pipeline data
    df = pd.concat(dataframes, ignore_index=True)
    
    batches = sorted(df['batch_size'].unique())
    
    # We want to group by pipeline type for better comparison
    pipelines = [
        "CPU Single-Threaded + Linux Sockets",
        "CPU Single-Threaded + DPDK",
        "CPU Multi-Threaded (12 threads) + DPDK",
        "GPU (ICMP) + DPDK",
        "CPU Heavy Payload (PRNG+CRC32) + DPDK",
        "GPU (Heavy Payload) + DPDK"
    ]
    
    # Filter only if they exist in data
    available_pipelines = [p for p in pipelines if p in df['pipeline'].unique()]

    plt.figure(figsize=(12, 7))
    
    bar_width = 0.12
    x = np.arange(len(batches))

    for i, pipe in enumerate(available_pipelines):
        pipe_data = df[df['pipeline'] == pipe]
        # Align with batches order
        pipe_data = pipe_data.set_index('batch_size').reindex(batches).reset_index()
        # Convert to KPps for readability
        plt.bar(x + i*bar_width, pipe_data['pps'] / 1000.0, bar_width, label=pipe)

    plt.xticks(x + bar_width * (len(available_pipelines)-1)/2, [str(b) for b in batches])
    plt.xlabel('Batch Size (Packets)')
    plt.ylabel('Throughput (KPps)')
    plt.title('End-to-End Pipeline Comparison: Kernel vs. Kernel-Bypass (DPDK)')
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    plt.tight_layout()
    plt.savefig('results/final_pipeline_comparison.png', dpi=300)
    plt.savefig('results/final_pipeline_comparison.pdf')
    print("Saved aggregated pipeline plots to results/")

if __name__ == "__main__":
    # Ensure results directory exists
    if not os.path.exists('results'):
        os.makedirs('results')
        
    plot_generation()
    plot_pipeline()
