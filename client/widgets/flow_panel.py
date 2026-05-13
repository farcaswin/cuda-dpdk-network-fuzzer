from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                             QListWidget, QGroupBox, QGridLayout, QListWidgetItem,
                             QPushButton)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QColor
import pyqtgraph as pg
from client.config import THEME, GRAPH
from client.api.models import FuzzStats
from datetime import datetime

class FlowPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._pps_data = [0] * GRAPH["total_points"]
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(15)

        # COMPONENT 1: Live PPS Graph
        graph_group = QGroupBox("Packet Generation Rate (GPU)")
        graph_layout = QVBoxLayout(graph_group)
        
        self.graph_widget = pg.PlotWidget()
        self.graph_widget.setBackground(THEME['bg_base'])
        self.graph_widget.showGrid(x=True, y=True, alpha=0.3)
        self.graph_widget.setLabel('left', 'Packets/sec')
        self.graph_widget.setLabel('bottom', 'Time (last 60s)')
        self.graph_widget.setYRange(0, 1000000) # Initial range to 1M pps
        
        self.curve = self.graph_widget.plot(
            pen=pg.mkPen(color=THEME['accent'], width=2),
            fillLevel=0,
            brush=pg.mkBrush(QColor(137, 180, 250, 50)) # Transparent accent blue
        )
        
        graph_layout.addWidget(self.graph_widget)
        layout.addWidget(graph_group)

        # COMPONENT 2: Counters Grid
        counters_group = QGroupBox("Live Statistics")
        grid = QGridLayout(counters_group)
        
        self.lbl_gen = self._create_stat_label("GENERATED", THEME['accent'])
        self.lbl_sent = self._create_stat_label("SENT", THEME['green'])
        self.lbl_pps = self._create_stat_label("PPS", THEME['yellow'])
        self.lbl_crashes = self._create_stat_label("CRASHES", THEME['red'])
        
        grid.addWidget(QLabel("GENERATED"), 0, 0)
        grid.addWidget(self.lbl_gen, 1, 0)
        grid.addWidget(QLabel("SENT"), 0, 1)
        grid.addWidget(self.lbl_sent, 1, 1)
        grid.addWidget(QLabel("CURRENT PPS"), 2, 0)
        grid.addWidget(self.lbl_pps, 3, 0)
        grid.addWidget(QLabel("CRASHES"), 2, 1)
        grid.addWidget(self.lbl_crashes, 3, 1)
        
        layout.addWidget(counters_group)

        # COMPONENT 3: Event Log
        log_group = QGroupBox("Event Log")
        log_layout = QVBoxLayout(log_group)
        
        self.event_log = QListWidget()
        self.event_log.setStyleSheet(f"background-color: {THEME['bg_base']}; border: none;")
        log_layout.addWidget(self.event_log)
        
        self.clear_log_btn = QPushButton("Clear Log")
        self.clear_log_btn.clicked.connect(self.event_log.clear)
        log_layout.addWidget(self.clear_log_btn)
        
        layout.addWidget(log_group)

        # COMPONENT 4: GPU Pipeline Visual
        gpu_group = QGroupBox("GPU Pipeline")
        gpu_layout = QVBoxLayout(gpu_group)
        
        self.buf_a_label = QLabel("Buffer A: [░░░░░░░░░░] Idle")
        self.buf_b_label = QLabel("Buffer B: [░░░░░░░░░░] Idle")
        self.gpu_mem_label = QLabel("GPU Memory: 0 MB / 4096 MB")
        
        gpu_layout.addWidget(self.buf_a_label)
        gpu_layout.addWidget(self.buf_b_label)
        gpu_layout.addWidget(self.gpu_mem_label)
        
        layout.addWidget(gpu_group)

    def _create_stat_label(self, name, color):
        lbl = QLabel("0")
        lbl.setStyleSheet(f"color: {color}; font-size: 18pt; font-weight: bold;")
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        return lbl

    def update_stats(self, stats: FuzzStats):
        # Update counters
        self.lbl_gen.setText(f"{stats.packets_generated:,}")
        self.lbl_sent.setText(f"{stats.packets_sent:,}")
        self.lbl_pps.setText(f"{stats.actual_pps:,}")
        self.lbl_crashes.setText(f"{stats.crashes_detected}")
        
        # Update Graph
        self._pps_data.pop(0)
        self._pps_data.append(stats.actual_pps)
        self.curve.setData(self._pps_data)
        
        # Adjust Y range if PPS is higher than current max
        current_max = max(self._pps_data)
        if current_max > 800000:
            self.graph_widget.setYRange(0, current_max * 1.2)
        else:
            self.graph_widget.setYRange(0, 1000000)

        # Approximate GPU visual
        fill_a = "█" * 10 if stats.packets_sent > 0 else "░" * 10
        self.buf_a_label.setText(f"Buffer A: [{fill_a}] Generating")
        self.buf_b_label.setText(f"Buffer B: [{fill_a}] Sending")

    def add_log_entry(self, level, msg):
        timestamp = datetime.now().strftime("%H:%M:%S")
        entry = f"[{timestamp}] {msg}"
        item = QListWidgetItem(entry)
        
        if level == "INFO": item.setForeground(QColor(THEME['text_sub']))
        elif level == "SEND": item.setForeground(QColor(THEME['accent']))
        elif level == "CRASH": 
            item.setForeground(QColor(THEME['red']))
            item.setText(f"⚠ {entry}")
        elif level == "RESTORE": item.setForeground(QColor(THEME['green']))
        
        self.event_log.addItem(item)
        self.event_log.scrollToBottom()
        
        if self.event_log.count() > 200:
            self.event_log.takeItem(0)

    def clear(self):
        self.lbl_gen.setText("0")
        self.lbl_sent.setText("0")
        self.lbl_pps.setText("0")
        self.lbl_crashes.setText("0")
        self._pps_data = [0] * GRAPH["total_points"]
        self.curve.setData(self._pps_data)
        self.event_log.clear()
