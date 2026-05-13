from PyQt6.QtCore import QThread, pyqtSignal

class BaseWorker(QThread):
    error = pyqtSignal(str)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._cancelled = False

    def cancel(self):
        self._cancelled = True

    def is_cancelled(self):
        return self._cancelled
