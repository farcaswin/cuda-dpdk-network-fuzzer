SERVER_URL = "http://localhost:8080"

# Catppuccin Mocha colors
THEME = {
    "bg_base":    "#1e1e2e",
    "bg_surface": "#313244", 
    "bg_overlay": "#45475a",
    "text":       "#cdd6f4",
    "text_sub":   "#a6adc8",
    "accent":     "#89b4fa",  # blue
    "green":      "#a6e3a1",
    "red":        "#f38ba8",
    "yellow":     "#f9e2af",
    "orange":     "#fab387",
}

POLL_INTERVALS = {
    "server_health":  3000,
    "vm_status":      2000,
    "vm_list":        5000,
    "fuzz_stats":      200,
}

STRATEGY_MAP = {
    "Throughput Test (ICMP)": "ICMP_TYPES",
    "IP Header Exhaustive":   "IP_HEADER",
    "TCP Flags Exhaustive":   "TCP_FLAGS",
}

BURST_SIZE_OPTIONS = [4096, 16384, 65536, 131072, 262144]

GRAPH = {
    "window_seconds": 60,
    "points_per_sec":  5,     # = poll every 200ms
    "total_points":   300,    # 60 * 5
}
