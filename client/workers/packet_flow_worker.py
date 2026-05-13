import time
from PyQt6.QtCore import pyqtSignal
from client.workers.base_worker import BaseWorker
from client.api.client import ApiClient
from client.api.models import FuzzStats

class PacketFlowWorker(BaseWorker):
    stats_update = pyqtSignal(object)
    crash_detected = pyqtSignal()
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._last_alive_state = True

    def run(self):
        while not self.is_cancelled():
            data, err = ApiClient.get_fuzz_status()
            
            if self.is_cancelled():
                break
                
            if not err and data:
                # Convert dict to FuzzStats model
                # Server uses: running, packets_sent, current_pps, target_alive
                try:
                    stats = FuzzStats(
                        packets_generated=data.get("packets_sent", 0), # Map sent to generated if not separate
                        packets_sent=data.get("packets_sent", 0),
                        actual_pps=int(data.get("current_pps", 0)),
                        crashes_detected=0, # This will be managed by UI/Monitor
                        target_alive=data.get("target_alive", True),
                        elapsed_sec=0, # Logic to be added if needed
                        strategy="",
                        batch_size=0
                    )
                    
                    self.stats_update.emit(stats)
                    
                    # Detect crash transition (True -> False)
                    if self._last_alive_state and not stats.target_alive:
                        self.crash_detected.emit()
                    
                    self._last_alive_state = stats.target_alive
                    
                except Exception as e:
                    print(f"Error parsing fuzz stats: {e}")
            
            # Poll every 200ms as per requirements
            time.sleep(0.2)
