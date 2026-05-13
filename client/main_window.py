import sys
from PyQt6.QtWidgets import QMainWindow, QSplitter, QVBoxLayout, QWidget, QStatusBar, QApplication
from PyQt6.QtCore import Qt, QTimer

from client.widgets.vm_panel import VMPanel
from client.widgets.fuzz_panel import FuzzPanel
from client.widgets.flow_panel import FlowPanel
from client.widgets.toolbar import AppToolbar
from client.config import THEME
from client.style import get_stylesheet
from client.workers.packet_flow_worker import PacketFlowWorker
from client.api.client import ApiClient
from client.workers.api_worker import ApiWorker

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("FuzzerServer Control — GPU Network Fuzzer")
        self.setMinimumSize(1280, 800)
        
        # Internal state
        self._workers = []
        self._flow_worker = None
        
        # Apply global stylesheet
        self.setStyleSheet(get_stylesheet())
        
        self.init_ui()
        self.connect_signals()
        
        # Initial UI State
        self._on_disconnected()

    def init_ui(self):
        # Central layout
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.main_layout = QVBoxLayout(self.central_widget)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_layout.setSpacing(0)
        
        # Toolbar
        self.toolbar = AppToolbar()
        self.main_layout.addWidget(self.toolbar)
        
        # Main Content with Splitter
        self.splitter = QSplitter(Qt.Orientation.Horizontal)
        
        self.vm_panel = VMPanel()
        self.fuzz_panel = FuzzPanel()
        self.flow_panel = FlowPanel()
        
        self.splitter.addWidget(self.vm_panel)
        self.splitter.addWidget(self.fuzz_panel)
        self.splitter.addWidget(self.flow_panel)
        
        # Set initial sizes (proportional)
        self.splitter.setStretchFactor(0, 1) # VM Panel
        self.splitter.setStretchFactor(1, 2) # Fuzz Panel
        self.splitter.setStretchFactor(2, 1) # Flow Panel
        
        self.main_layout.addWidget(self.splitter)
        
        # Status Bar
        self.setStatusBar(QStatusBar())
        self.statusBar().showMessage("Ready")
        self.statusBar().setStyleSheet(f"background-color: {THEME['bg_surface']}; border-top: 1px solid {THEME['bg_overlay']};")

    def connect_signals(self):
        # Toolbar signals
        self.toolbar.connected.connect(self._on_connected)
        self.toolbar.disconnected.connect(self._on_disconnected)
        self.toolbar.shutdown_requested.connect(self._on_shutdown)
        
        # VM Panel signals
        self.vm_panel.target_changed.connect(self.fuzz_panel.set_target)
        self.vm_panel.status_updated.connect(self.fuzz_panel.update_target_status)
        self.vm_panel.vm_started.connect(lambda vm_id: self.flow_panel.add_log_entry("INFO", f"VM {vm_id} started"))
        self.vm_panel.vm_stopped.connect(lambda vm_id: self.flow_panel.add_log_entry("INFO", f"VM {vm_id} stopped"))
        
        # Fuzz Panel signals
        self.fuzz_panel.fuzz_started.connect(self._on_fuzz_started)
        self.fuzz_panel.fuzz_stopped.connect(self._on_fuzz_stopped)
        self.fuzz_panel.crash_detected.connect(self._on_crash)

    def _on_connected(self):
        self.vm_panel.setEnabled(True)
        self.fuzz_panel.setEnabled(True)
        self.flow_panel.setEnabled(True)
        self.statusBar().showMessage("Connected to FuzzerServer")
        self.flow_panel.add_log_entry("INFO", "Connected to server")
        self.vm_panel.refresh_profiles()

    def _on_disconnected(self):
        self.vm_panel.setEnabled(False)
        self.fuzz_panel.setEnabled(False)
        self.flow_panel.setEnabled(False)
        self.statusBar().showMessage("Disconnected from server")
        if self._flow_worker:
            self._on_fuzz_stopped()

    def _on_shutdown(self):
        self.flow_panel.add_log_entry("INFO", "Server shutdown requested...")
        self.statusBar().showMessage("Server shutting down...")

    def _on_fuzz_started(self, config):
        self.statusBar().showMessage(f"FUZZING ACTIVE: {config['strategy']} @ {config['rate_pps']:,} PPS")
        self.flow_panel.add_log_entry("INFO", f"Fuzzing session started: {config['strategy']}")
        
        # Start high-frequency stats polling
        self._flow_worker = PacketFlowWorker()
        self._flow_worker.stats_update.connect(self.fuzz_panel.update_stats)
        self._flow_worker.stats_update.connect(self.flow_panel.update_stats)
        self._flow_worker.crash_detected.connect(self.fuzz_panel.on_crash)
        self._flow_worker.crash_detected.connect(self._on_crash)
        self._flow_worker.start()
        self._workers.append(self._flow_worker)

    def _on_fuzz_stopped(self):
        self.statusBar().showMessage("Fuzzing stopped")
        self.flow_panel.add_log_entry("INFO", "Fuzzing session stopped")
        if self._flow_worker:
            self._flow_worker.cancel()
            self._flow_worker = None

    def _on_crash(self):
        self.statusBar().showMessage("⚠️ TARGET CRASHED!")
        self.flow_panel.add_log_entry("CRASH", "Network stack crash detected on target!")
        QApplication.beep()

    def closeEvent(self, event):
        # Clean up workers before closing
        if self._flow_worker:
            self._flow_worker.cancel()
        
        for worker in self._workers:
            if worker.isRunning():
                worker.cancel()
                worker.wait(1000)
        
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
