from PyQt6.QtCore import pyqtSignal
from client.workers.base_worker import BaseWorker

class ApiWorker(BaseWorker):
    finished = pyqtSignal(object)
    
    def __init__(self, fn, parent=None):
        """
        :param fn: A callable that returns (data, error_msg)
        """
        super().__init__(parent)
        self.fn = fn

    def run(self):
        if self.is_cancelled():
            return
            
        result, err = self.fn()
        
        if self.is_cancelled():
            return

        if err:
            self.error.emit(err)
        else:
            self.finished.emit(result)
