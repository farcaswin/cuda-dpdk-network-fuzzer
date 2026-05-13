from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                             QPushButton, QListWidget, QGroupBox, QListWidgetItem,
                             QMessageBox)
from PyQt6.QtCore import pyqtSignal, Qt
from client.config import THEME, POLL_INTERVALS
from client.api.client import ApiClient
from client.api.models import VMProfile, VMStatus
from client.workers.api_worker import ApiWorker
from client.workers.poll_worker import PollWorker

class VMPanel(QWidget):
    vm_selected = pyqtSignal(str)              # vm_id
    target_changed = pyqtSignal(object)         # VMProfile
    vm_started = pyqtSignal(str)               # vm_id
    vm_stopped = pyqtSignal(str)               # vm_id
    status_updated = pyqtSignal(str, dict)     # vm_id, status_data

    def __init__(self, parent=None):
        super().__init__(parent)
        self._workers = []
        self._profiles = {} # vm_id -> VMProfile
        self._active_vm_id = None
        self._status_worker = None
        
        self.init_ui()
        self.refresh_profiles()

    def init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(15)

        # SECTION 1: VM Profiles
        header_layout = QHBoxLayout()
        label = QLabel("VM Targets")
        label.setObjectName("section_header")
        header_layout.addWidget(label)
        
        self.refresh_btn = QPushButton("⟳")
        self.refresh_btn.setFixedWidth(30)
        self.refresh_btn.clicked.connect(self.refresh_profiles)
        header_layout.addWidget(self.refresh_btn)
        layout.addLayout(header_layout)

        self.profile_list = QListWidget()
        self.profile_list.itemDoubleClicked.connect(self._on_item_double_clicked)
        layout.addWidget(self.profile_list)

        # SECTION 2: Selected VM Controls
        self.controls_group = QGroupBox("Controls")
        self.controls_group.setEnabled(False)
        controls_layout = QVBoxLayout(self.controls_group)
        
        btn_layout = QHBoxLayout()
        self.start_btn = QPushButton("▶ Start")
        self.start_btn.setStyleSheet(f"background-color: {THEME['green']}; color: {THEME['bg_base']}; font-weight: bold;")
        self.start_btn.clicked.connect(self.start_vm)
        
        self.stop_btn = QPushButton("■ Stop")
        self.stop_btn.setStyleSheet(f"background-color: {THEME['red']}; color: {THEME['bg_base']}; font-weight: bold;")
        self.stop_btn.clicked.connect(self.stop_vm)
        
        btn_layout.addWidget(self.start_btn)
        btn_layout.addWidget(self.stop_btn)
        controls_layout.addLayout(btn_layout)

        self.status_label = QLabel("Status: Unknown")
        self.ip_label = QLabel("IP: ---")
        self.mac_label = QLabel("MAC: ---")
        controls_layout.addWidget(self.status_label)
        controls_layout.addWidget(self.ip_label)
        controls_layout.addWidget(self.mac_label)
        layout.addWidget(self.controls_group)

        # SECTION 3: Snapshots
        self.snapshot_group = QGroupBox("Snapshots")
        self.snapshot_group.setEnabled(False)
        snap_layout = QVBoxLayout(self.snapshot_group)
        
        self.snapshots_list = QListWidget()
        snap_layout.addWidget(self.snapshots_list)
        
        snap_btn_layout = QHBoxLayout()
        self.snap_create_btn = QPushButton("📷 Create")
        self.snap_create_btn.clicked.connect(self.create_snapshot)
        
        self.snap_restore_btn = QPushButton("⏮ Restore")
        self.snap_restore_btn.clicked.connect(self.restore_snapshot)
        
        snap_btn_layout.addWidget(self.snap_create_btn)
        snap_btn_layout.addWidget(self.snap_restore_btn)
        snap_layout.addLayout(snap_btn_layout)
        layout.addWidget(self.snapshot_group)

    def refresh_profiles(self):
        worker = ApiWorker(ApiClient.get_profiles)
        worker.finished.connect(self._on_profiles_loaded)
        worker.error.connect(lambda e: print(f"Error loading profiles: {e}"))
        worker.start()
        self._workers.append(worker)

    def _on_profiles_loaded(self, data):
        self.profile_list.clear()
        self._profiles = {}
        for p_data in data:
            profile = VMProfile(
                id=p_data['id'],
                name=p_data.get('name', p_data['id']),
                display_name=p_data['display_name'],
                target_ip=p_data.get('target_ip', '0.0.0.0'),
                target_mac=p_data.get('target_mac', '00:00:00:00:00:00'),
                memory_mb=p_data.get('memory_mb', 1024),
                vcpus=p_data.get('vcpus', 1),
                disk_path=p_data.get('disk_image_name', ''),
                strategies=p_data.get('recommended_strategies', []),
                os_family=p_data.get('os_family', 'linux')
            )
            self._profiles[profile.id] = profile
            item = QListWidgetItem(profile.display_name)
            item.setData(Qt.ItemDataRole.UserRole, profile.id)
            self.profile_list.addItem(item)

    def _on_item_double_clicked(self, item):
        vm_id = item.data(Qt.ItemDataRole.UserRole)
        self._active_vm_id = vm_id
        profile = self._profiles[vm_id]
        
        self.controls_group.setEnabled(True)
        self.snapshot_group.setEnabled(True)
        self.vm_selected.emit(vm_id)
        self.target_changed.emit(profile)
        
        self.refresh_snapshots()
        self.start_status_polling()

    def start_status_polling(self):
        if self._status_worker:
            self._status_worker.cancel()
        
        self._status_worker = PollWorker(lambda: ApiClient.get_vm_status(self._active_vm_id), 2000)
        self._status_worker.update.connect(self._on_status_update)
        self._status_worker.start()
        self._workers.append(self._status_worker)

    def _on_status_update(self, data):
        state = data.get("state", "unknown")
        ip = data.get("ip", "---")
        mac = data.get("mac", "---")
        
        color = THEME['red'] if state != "running" else THEME['green']
        self.status_label.setText(f"Status: <b style='color: {color}'>{state.upper()}</b>")
        self.ip_label.setText(f"IP: {ip}")
        self.mac_label.setText(f"MAC: {mac}")
        
        self.status_updated.emit(self._active_vm_id, data)
        
        self.start_btn.setEnabled(state != "running")
        self.stop_btn.setEnabled(state == "running")

    def start_vm(self):
        worker = ApiWorker(lambda: ApiClient.post_vm_start(self._active_vm_id))
        worker.finished.connect(lambda _: self.vm_started.emit(self._active_vm_id))
        worker.error.connect(lambda e: QMessageBox.critical(self, "Error", f"Failed to start VM: {e}"))
        worker.start()
        self._workers.append(worker)

    def stop_vm(self):
        worker = ApiWorker(lambda: ApiClient.post_vm_stop(self._active_vm_id))
        worker.finished.connect(lambda _: self.vm_stopped.emit(self._active_vm_id))
        worker.error.connect(lambda e: QMessageBox.critical(self, "Error", f"Failed to stop VM: {e}"))
        worker.start()
        self._workers.append(worker)

    def refresh_snapshots(self):
        worker = ApiWorker(lambda: ApiClient.get_snapshots(self._active_vm_id))
        worker.finished.connect(self._on_snapshots_loaded)
        worker.start()
        self._workers.append(worker)

    def _on_snapshots_loaded(self, data):
        self.snapshots_list.clear()
        for snap in data:
            name = snap if isinstance(snap, str) else snap.get("name", "Unknown")
            self.snapshots_list.addItem(name)

    def create_snapshot(self):
        worker = ApiWorker(lambda: ApiClient.post_snapshot(self._active_vm_id))
        worker.finished.connect(lambda _: self.refresh_snapshots())
        worker.error.connect(lambda e: QMessageBox.critical(self, "Error", f"Failed to create snapshot: {e}"))
        worker.start()
        self._workers.append(worker)

    def restore_snapshot(self):
        selected = self.snapshots_list.currentItem()
        name = selected.text() if selected else None
        
        worker = ApiWorker(lambda: ApiClient.post_restore(self._active_vm_id, name))
        worker.finished.connect(lambda _: QMessageBox.information(self, "Success", "Snapshot restored!"))
        worker.error.connect(lambda e: QMessageBox.critical(self, "Error", f"Failed to restore: {e}"))
        worker.start()
        self._workers.append(worker)
