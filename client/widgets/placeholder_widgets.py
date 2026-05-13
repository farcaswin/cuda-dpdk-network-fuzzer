from PyQt6.QtWidgets import QWidget, QVBoxLayout, QLabel
from client.config import THEME

class BasePanel(QWidget):
    def __init__(self, title, parent=None):
        super().__init__(parent)
        self.layout = QVBoxLayout(self)
        self.label = QLabel(title)
        self.label.setStyleSheet(f"color: {THEME['accent']}; font-weight: bold; font-size: 14pt;")
        self.layout.addWidget(self.label)
        self.layout.addStretch()
        self.setStyleSheet(f"background-color: {THEME['bg_surface']}; border: 1px solid {THEME['bg_overlay']};")

class VMPanel(BasePanel):
    def __init__(self, parent=None):
        super().__init__("VM Targets", parent)

class FuzzPanel(BasePanel):
    def __init__(self, parent=None):
        super().__init__("Fuzzing Control", parent)

class FlowPanel(BasePanel):
    def __init__(self, parent=None):
        super().__init__("Live Packet Flow", parent)
