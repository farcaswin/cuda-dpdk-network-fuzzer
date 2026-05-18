from dataclasses import dataclass, field
from typing import List, Optional

@dataclass
class VMProfile:
    id: str
    name: str
    display_name: str
    target_ip: str
    target_mac: str
    memory_mb: int
    vcpus: int
    disk_path: str
    strategies: List[str] = field(default_factory=list)
    os_family: str = "linux"

@dataclass
class FuzzStats:
    packets_generated: int = 0
    packets_sent: int = 0
    actual_pps: int = 0
    crashes_detected: int = 0
    target_alive: bool = True
    is_running: bool = True
    elapsed_sec: int = 0
    strategy: str = ""
    batch_size: int = 0

@dataclass
class VMStatus:
    name: str
    state: str
    ip: str = ""
    mac: str = ""

@dataclass
class SnapshotInfo:
    name: str
    description: str = ""
    creation_time: str = ""
