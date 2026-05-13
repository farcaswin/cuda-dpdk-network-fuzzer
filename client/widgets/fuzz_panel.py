from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                             QPushButton, QGroupBox, QComboBox, QSlider, 
                             QSpinBox, QCheckBox, QProgressBar)
from PyQt6.QtCore import pyqtSignal, Qt, QPropertyAnimation, QEasingCurve
from client.config import THEME, STRATEGY_MAP, BURST_SIZE_OPTIONS
from client.api.client import ApiClient
from client.api.models import VMProfile, FuzzStats
from client.workers.api_worker import ApiWorker
from client.workers.packet_flow_worker import PacketFlowWorker

class FuzzPanel(QWidget):
    fuzz_started = pyqtSignal(dict)
    fuzz_stopped = pyqtSignal()
    crash_detected = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._workers = []
        self._flow_worker = None
        self._active_profile = None
        self._is_fuzzing = False
        
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(15)

        # SECTION 1: Target Info
        self.target_group = QGroupBox("Active Target")
        target_layout = QVBoxLayout(self.target_group)
        self.target_name = QLabel("Target: None")
        self.target_name.setStyleSheet("font-weight: bold; font-size: 11pt;")
        self.target_details = QLabel("IP: --- | MAC: ---")
        
        self.status_badge = QLabel("OFFLINE")
        self.status_badge.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_badge.setFixedHeight(30)
        self.status_badge.setStyleSheet(f"background-color: {THEME['bg_overlay']}; border-radius: 4px; font-weight: bold;")
        
        target_layout.addWidget(self.target_name)
        target_layout.addWidget(self.target_details)
        target_layout.addWidget(self.status_badge)
        layout.addWidget(self.target_group)

        # SECTION 2: Configuration
        self.config_group = QGroupBox("Fuzzing Configuration")
        config_layout = QVBoxLayout(self.config_group)

        # Strategy
        strat_layout = QHBoxLayout()
        strat_layout.addWidget(QLabel("Strategy:"))
        self.strategy_combo = QComboBox()
        self.strategy_combo.addItems(STRATEGY_MAP.keys())
        strat_layout.addWidget(self.strategy_combo, 1)
        config_layout.addLayout(strat_layout)

        # Rate Slider
        rate_layout = QVBoxLayout()
        rate_header = QHBoxLayout()
        rate_header.addWidget(QLabel("Rate (pps):"))
        self.rate_label = QLabel("10,000")
        self.rate_label.setStyleSheet(f"color: {THEME['accent']}; font-weight: bold;")
        rate_header.addStretch()
        rate_header.addWidget(self.rate_label)
        rate_layout.addLayout(rate_header)
        
        self.rate_slider = QSlider(Qt.Orientation.Horizontal)
        self.rate_slider.setRange(1, 100) # 10k to 1M
        self.rate_slider.setValue(10)
        self.rate_slider.valueChanged.connect(self._on_rate_changed)
        rate_layout.addWidget(self.rate_slider)
        config_layout.addLayout(rate_layout)

        # Batch & Duration
        row3 = QHBoxLayout()
        row3.addWidget(QLabel("Batch Size:"))
        self.batch_combo = QComboBox()
        self.batch_combo.addItems([str(x) for x in BURST_SIZE_OPTIONS])
        self.batch_combo.setCurrentText("65536")
        row3.addWidget(self.batch_combo, 1)
        
        row3.addWidget(QLabel("Duration (s):"))
        self.duration_spin = QSpinBox()
        self.duration_spin.setRange(0, 3600)
        self.duration_spin.setValue(60)
        row3.addWidget(self.duration_spin, 1)
        config_layout.addLayout(row3)

        # Options
        self.auto_snapshot = QCheckBox("Create snapshot before fuzzing")
        self.auto_snapshot.setChecked(True)
        self.auto_restore = QCheckBox("Auto-restore on crash")
        self.auto_restore.setChecked(True)
        config_layout.addWidget(self.auto_snapshot)
        config_layout.addWidget(self.auto_restore)
        
        layout.addWidget(self.config_group)

        # SECTION 3: Control Button
        self.start_btn = QPushButton("▶  START FUZZING")
        self.start_btn.setObjectName("start_button")
        self.start_btn.setFixedHeight(50)
        self.start_btn.setEnabled(False)
        self.start_btn.clicked.connect(self.toggle_fuzzing)
        layout.addWidget(self.start_btn)

        # SECTION 4: Stats
        self.stats_group = QGroupBox("Session Statistics")
        self.stats_group.hide()
        stats_layout = QVBoxLayout(self.stats_group)
        
        self.stat_pkts = QLabel("Packets Sent: 0")
        self.stat_pps = QLabel("Current PPS: 0")
        self.stat_crashes = QLabel("Crashes: 0")
        self.progress = QProgressBar()
        
        stats_layout.addWidget(self.stat_pkts)
        stats_layout.addWidget(self.stat_pps)
        stats_layout.addWidget(self.stat_crashes)
        stats_layout.addWidget(self.progress)
        layout.addWidget(self.stats_group)

        layout.addStretch()

    def set_target(self, profile: VMProfile):
        self._active_profile = profile
        self.target_name.setText(f"Target: {profile.display_name}")
        self.target_details.setText(f"IP: {profile.target_ip} | MAC: {profile.target_mac}")
        self.status_badge.setText("ALIVE")
        self.status_badge.setStyleSheet(f"background-color: {THEME['green']}; color: {THEME['bg_base']}; font-weight: bold;")
        self.start_btn.setEnabled(True)

    def update_target_status(self, vm_id, data):
        if self._active_profile and self._active_profile.id == vm_id:
            ip = data.get("ip", "---")
            mac = data.get("mac", "---")
            state = data.get("state", "unknown")
            
            # Update the profile object with live data so start_fuzzing uses it
            self._active_profile.target_ip = ip
            self._active_profile.target_mac = mac
            
            self.target_details.setText(f"IP: {ip} | MAC: {mac}")
            
            if state == "running":
                self.status_badge.setText("ALIVE")
                self.status_badge.setStyleSheet(f"background-color: {THEME['green']}; color: {THEME['bg_base']}; font-weight: bold;")
            else:
                self.status_badge.setText("OFFLINE")
                self.status_badge.setStyleSheet(f"background-color: {THEME['bg_overlay']}; border-radius: 4px; font-weight: bold;")

    def _on_rate_changed(self, val):
        pps = val * 10000
        self.rate_label.setText(f"{pps:,}")

    def toggle_fuzzing(self):
        if not self._is_fuzzing:
            self.start_fuzzing()
        else:
            self.stop_fuzzing()

    def start_fuzzing(self):
        config = {
            "vm_id": self._active_profile.id,
            "target_ip": self._active_profile.target_ip,
            "target_mac": self._active_profile.target_mac,
            "strategy": STRATEGY_MAP[self.strategy_combo.currentText()],
            "rate_pps": self.rate_slider.value() * 10000,
            "batch_size": int(self.batch_combo.currentText()),
            "duration_sec": self.duration_spin.value(),
            "auto_snapshot": self.auto_snapshot.isChecked(),
            "auto_restore": self.auto_restore.isChecked()
        }
        
        worker = ApiWorker(lambda: ApiClient.post_fuzz_start(config))
        worker.finished.connect(lambda _: self._on_fuzz_started(config))
        worker.error.connect(lambda e: print(f"Fuzz error: {e}"))
        worker.start()
        self._workers.append(worker)

    def _on_fuzz_started(self, config):
        self._is_fuzzing = True
        self.start_btn.setText("■  STOP FUZZING")
        self.start_btn.setObjectName("stop_button")
        self.start_btn.setStyleSheet(f"background-color: {THEME['red']};") # Manual override as style.py needs re-polish
        self.stats_group.show()
        self.config_group.setEnabled(False)
        self.fuzz_started.emit(config)

    def stop_fuzzing(self):
        worker = ApiWorker(ApiClient.post_fuzz_stop)
        worker.finished.connect(self._on_fuzz_stopped)
        worker.start()
        self._workers.append(worker)

    def _on_fuzz_stopped(self):
        self._is_fuzzing = False
        self.start_btn.setText("▶  START FUZZING")
        self.start_btn.setObjectName("start_button")
        self.start_btn.setStyleSheet(f"background-color: {THEME['green']};")
        self.config_group.setEnabled(True)
        self.fuzz_stopped.emit()

    def update_stats(self, stats: FuzzStats):
        self.stat_pkts.setText(f"Packets Sent: {stats.packets_sent:,}")
        self.stat_pps.setText(f"Current PPS: {stats.actual_pps:,}")
        self.stat_crashes.setText(f"Crashes Detected: {stats.crashes_detected}")
        
        if self.duration_spin.value() > 0:
            val = int((stats.elapsed_sec / self.duration_spin.value()) * 100)
            self.progress.setValue(min(val, 100))
        
        if not stats.target_alive:
            self.on_crash()
        else:
            self.status_badge.setText("ALIVE")
            self.status_badge.setStyleSheet(f"background-color: {THEME['green']}; color: {THEME['bg_base']}; font-weight: bold;")

    def on_crash(self):
        self.status_badge.setText("✕ CRASHED")
        self.status_badge.setStyleSheet(f"background-color: {THEME['red']}; color: {THEME['bg_base']}; font-weight: bold;")
        self.crash_detected.emit()
