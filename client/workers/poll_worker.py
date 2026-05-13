import time
from PyQt6.QtCore import pyqtSignal
from client.workers.base_worker import BaseWorker

class PollWorker(BaseWorker):
    update = pyqtSignal(object)
    
    def __init__(self, fn, interval_ms, parent=None):
        """
        :param fn: A callable that returns (data, error_msg)
        :param interval_ms: Polling interval in milliseconds
        """
        super().__init__(parent)
        self.fn = fn
        self.interval_ms = interval_ms

    def run(self):
        while not self.is_cancelled():
            result, err = self.fn()
            
            if self.is_cancelled():
                break
                
            if not err:
                self.update.emit(result)
            else:
                self.error.emit(err)
            
            # Sleep in small increments to remain responsive to cancel()
            for _ in range(int(self.interval_ms / 100)):
                if self.is_cancelled():
                    break
                time.sleep(0.1)
