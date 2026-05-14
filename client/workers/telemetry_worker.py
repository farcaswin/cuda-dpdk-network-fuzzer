import asyncio
import json
import websockets
from PyQt6.QtCore import pyqtSignal
from client.workers.base_worker import BaseWorker
from client.api.models import FuzzStats
from client.config import SERVER_URL

class TelemetryWorker(BaseWorker):
    stats_update = pyqtSignal(object)
    crash_detected = pyqtSignal(str)
    log_received = pyqtSignal(str, str)
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    
    def __init__(self, parent=None):
        super().__init__(parent)
        # Convert http://host:port to ws://host:port/api/telemetry
        ws_base = SERVER_URL.replace("http://", "ws://")
        self.ws_url = f"{ws_base}/api/telemetry"

    def run(self):
        # We need to run the async event loop in this thread
        asyncio.run(self._listen())

    async def _listen(self):
        while not self.is_cancelled():
            try:
                async with websockets.connect(self.ws_url) as websocket:
                    self.connected.emit()
                    print(f"Connected to telemetry: {self.ws_url}")
                    while not self.is_cancelled():
                        try:
                            # Use wait_for to check cancel flag periodically
                            message = await asyncio.wait_for(websocket.recv(), timeout=1.0)
                            self._handle_message(message)
                        except asyncio.TimeoutError:
                            continue
                        except websockets.exceptions.ConnectionClosed:
                            self.disconnected.emit()
                            break
            except Exception as e:
                if not self.is_cancelled():
                    self.disconnected.emit()
                    print(f"WebSocket error: {e}. Retrying in 2s...")
                    await asyncio.sleep(2)
                else:
                    break

    def _handle_message(self, message):
        try:
            payload = json.loads(message)
            msg_type = payload.get("type")
            
            if msg_type == "stats":
                data = payload.get("data", {})
                stats = FuzzStats(
                    packets_generated=data.get("packets_sent", 0),
                    packets_sent=data.get("packets_sent", 0),
                    actual_pps=int(data.get("pps", 0)),
                    target_alive=data.get("target_alive", True)
                )
                self.stats_update.emit(stats)
            
            elif msg_type == "crash":
                vm_id = payload.get("vm_id", "unknown")
                self.crash_detected.emit(vm_id)
                
            elif msg_type == "log":
                level = payload.get("level", "INFO")
                text = payload.get("message", "")
                self.log_received.emit(level, text)
                
        except Exception as e:
            print(f"Error handling WS message: {e}")
