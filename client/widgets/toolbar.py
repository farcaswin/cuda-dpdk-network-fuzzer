from PyQt6.QtWidgets import QWidget, QHBoxLayout, QLabel, QPushButton, QMessageBox
from PyQt6.QtCore import pyqtSignal, Qt
from client.config import THEME, POLL_INTERVALS
from client.api.client import ApiClient
from client.workers.poll_worker import PollWorker
from client.workers.api_worker import ApiWorker

class AppToolbar(QWidget):
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    shutdown_requested = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedHeight(60)
        self.setObjectName("toolbar")
        self._workers = []
        self._is_connected = False
        self._health_worker = None
        
        self.init_ui()

    def init_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(15, 0, 15, 0)
        
        # LEFT SECTION: Title and URL
        left_layout = QHBoxLayout()
        self.title_label = QLabel("FuzzerServer")
        self.title_label.setObjectName("section_header")
        self.title_label.setStyleSheet(f"color: {THEME['accent']}; font-size: 16pt; font-weight: bold; margin-right: 15px;")
        left_layout.addWidget(self.title_label)
        
        self.url_label = QLabel(ApiClient.SERVER_URL if hasattr(ApiClient, "SERVER_URL") else "http://localhost:8080")
        self.url_label.setStyleSheet(f"color: {THEME['text_sub']}; font-family: monospace;")
        left_layout.addWidget(self.url_label)
        layout.addLayout(left_layout)
        
        layout.addStretch()
        
        # CENTER SECTION: Status and Connect
        center_layout = QHBoxLayout()
        self.status_indicator = QLabel("● Disconnected")
        self.status_indicator.setStyleSheet(f"color: {THEME['red']}; font-weight: bold; margin-right: 10px;")
        center_layout.addWidget(self.status_indicator)
        
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.setFixedWidth(100)
        self.connect_btn.clicked.connect(self.toggle_connection)
        center_layout.addWidget(self.connect_btn)
        layout.addLayout(center_layout)
        
        layout.addStretch()
        
        # RIGHT SECTION: Shutdown
        right_layout = QHBoxLayout()
        self.shutdown_btn = QPushButton("Shutdown Server")
        self.shutdown_btn.setObjectName("danger_button")
        self.shutdown_btn.setFixedWidth(140)
        self.shutdown_btn.clicked.connect(self.confirm_shutdown)
        right_layout.addWidget(self.shutdown_btn)
        layout.addLayout(right_layout)

    def toggle_connection(self):
        if not self._is_connected:
            self.start_connection()
        else:
            self.stop_connection()

    def start_connection(self):
        print("DEBUG: Starting connection...")
        self.status_indicator.setText("● Connecting...")
        self.status_indicator.setStyleSheet(f"color: {THEME['yellow']}; font-weight: bold; margin-right: 10px;")
        self.connect_btn.setEnabled(False)
        
        # Start health polling
        self._health_worker = PollWorker(ApiClient.get_health, POLL_INTERVALS["server_health"])
        self._health_worker.update.connect(self._on_health_update)
        self._health_worker.error.connect(self._on_health_error)
        self._health_worker.start()
        # Ensure reference to prevent GC
        self._workers.append(self._health_worker)

    def stop_connection(self):
        print("DEBUG: Stopping connection...")
        if self._health_worker:
            self._health_worker.cancel()
            self._health_worker = None
        
        self._is_connected = False
        self.status_indicator.setText("● Disconnected")
        self.status_indicator.setStyleSheet(f"color: {THEME['red']}; font-weight: bold; margin-right: 10px;")
        self.connect_btn.setText("Connect")
        self.connect_btn.setEnabled(True)
        self.disconnected.emit()

    def _on_health_update(self, data):
        print(f"DEBUG: Health update: {data}")
        if not self._is_connected:
            self._is_connected = True
            self.status_indicator.setText("● Connected")
            self.status_indicator.setStyleSheet(f"color: {THEME['green']}; font-weight: bold; margin-right: 10px;")
            self.connect_btn.setText("Disconnect")
            self.connect_btn.setEnabled(True)
            self.connected.emit()

    def _on_health_error(self, err):
        # We print error only when we are NOT connected to avoid spamming console
        # but for debug, we print it once if we were expecting connection
        if not self._is_connected:
             print(f"DEBUG: Health error during connecting: {err}")
        
        if self._is_connected:
            print(f"DEBUG: Connection lost: {err}")
            self._is_connected = False
            self.status_indicator.setText("● Disconnected")
            self.status_indicator.setStyleSheet(f"color: {THEME['red']}; font-weight: bold; margin-right: 10px;")
            self.connect_btn.setText("Connect")
            self.disconnected.emit()

    def confirm_shutdown(self):
        reply = QMessageBox.question(
            self, "Confirm Shutdown",
            "Are you sure you want to shut down the FuzzerServer?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            worker = ApiWorker(ApiClient.post_shutdown)
            worker.finished.connect(lambda: self.stop_connection())
            worker.start()
            self._workers.append(worker)
            self.shutdown_requested.emit()
