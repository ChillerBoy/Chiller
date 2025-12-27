import sys, json, threading, time
from dataclasses import dataclass
from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QLabel,
    QGroupBox, QTabWidget, QPushButton, QLineEdit, QComboBox, QTextEdit, QSlider
)
from PySide6.QtCore import QTimer, Qt
import serial

# -----------------------------
# PATCH_POINT_SERIAL
# -----------------------------
SERIAL_PORT = "COM3"   # Windows example. Pi/Linux: "/dev/ttyACM0"
BAUD = 115200

@dataclass(frozen=True)
class Field:
    key: str
    label: str
    units: str

# Display fields (subset; Arduino still publishes everything)
TEMP_FIELDS = [
    Field("EVAP_LWT_F", "Evap LWT", "°F"),
    Field("EVAP_EWT_F", "Evap EWT", "°F"),
    Field("COND_LWT_F", "Cond LWT", "°F"),
    Field("COND_EWT_F", "Cond EWT", "°F"),
    Field("SUCTION_F", "Suction Temp", "°F"),
    Field("DISCHARGE_F", "Discharge Temp", "°F"),
]
PRESS_FIELDS = [
    Field("P_SUCTION", "Suction P", "psig"),
    Field("P_COMP_DISCH", "Disch P", "psig"),
    Field("P_LIQUID_LINE", "Liquid Line P", "psig"),
]
FLOW_FIELDS = [
    Field("F_EVAP_GPM", "Evap Flow", "GPM"),
    Field("F_COND_GPM", "Cond Flow", "GPM"),
    Field("F_AHU_GPM", "AHU Flow", "GPM"),
]

ALARM_FIELDS = [
    Field("ALM_EvapFlowFailToProve", "Evap Flow Fail to Prove", ""),
    Field("ALM_EvapFlowLost", "Evap Flow Lost", ""),
    Field("ALM_CondFlowFailToProve", "Cond Flow Fail to Prove", ""),
    Field("ALM_CondFlowLost", "Cond Flow Lost", ""),
    Field("ALM_HiDischP", "High Discharge P", ""),
    Field("ALM_LoSuctionP", "Low Suction P", ""),
    Field("ALM_VFDFault", "VFD Fault", ""),
    Field("ALM_SensorFault", "Sensor Fault", ""),
    Field("ALM_TrainingMode", "Training Mode", ""),
]

def fmt(v, nd=1):
    if v is None:
        return "--"
    try:
        if isinstance(v, bool):
            return "ON" if v else "OFF"
        return f"{float(v):0.{nd}f}"
    except Exception:
        return str(v)

class ChillerHMI(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Chiller HMI - Tabs + Manual Control + Calibration")

        self.ser = None
        self.serial_lock = threading.Lock()
        self.running = True
        self.latest = {}
        self.rx_errors = 0

        root = QVBoxLayout(self)

        # -----------------------------
        # Top control bar
        # -----------------------------
        top = QHBoxLayout()
        self.lbl_port = QLabel(f"Port: {SERIAL_PORT} @ {BAUD}")
        top.addWidget(self.lbl_port)

        top.addWidget(QLabel("Mode:"))
        self.cmb_mode = QComboBox()
        self.cmb_mode.addItems(["AUTO", "SERVICE"])
        self.btn_set_mode = QPushButton("Apply")
        self.btn_set_mode.clicked.connect(self.apply_mode)
        top.addWidget(self.cmb_mode)
        top.addWidget(self.btn_set_mode)

        self.btn_reset = QPushButton("RESET (Clear Alarms)")
        self.btn_reset.clicked.connect(lambda: self.send("RESET"))
        top.addWidget(self.btn_reset)

        self.lbl_status = QLabel("Disconnected")
        self.lbl_status.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        top.addWidget(self.lbl_status, stretch=1)

        root.addLayout(top)

        # -----------------------------
        # Tabs
        # -----------------------------
        self.tabs = QTabWidget()
        root.addWidget(self.tabs)

        self.tabs.addTab(self.build_overview_tab(), "Overview")
        self.tabs.addTab(self.build_evap_tab(), "Evaporator")
        self.tabs.addTab(self.build_cond_tab(), "Condenser")
        self.tabs.addTab(self.build_refrig_tab(), "Refrigerant")
        self.tabs.addTab(self.build_tower_tab(), "Cooling Tower")
        self.tabs.addTab(self.build_vfd_tab(), "Compressor / VFD")
        self.tabs.addTab(self.build_alarms_tab(), "Alarms & Events")
        self.tabs.addTab(self.build_training_tab(), "Training / Simulation")
        self.tabs.addTab(self.build_calibration_tab(), "Calibration")

        # -----------------------------
        # Serial background reader
        # -----------------------------
        self.open_serial()
        self.reader_thread = threading.Thread(target=self.serial_reader_loop, daemon=True)
        self.reader_thread.start()

        # UI refresh timer
        self.timer = QTimer()
        self.timer.timeout.connect(self.refresh)
        self.timer.start(250)

    # -----------------------------
    # Serial
    # -----------------------------
    def open_serial(self):
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD, timeout=0.1)
            self.lbl_status.setText("Connected")
        except Exception as e:
            self.ser = None
            self.lbl_status.setText(f"Disconnected: {e}")

    def send(self, line: str):
        line = (line or "").strip()
        if not line:
            return
        if not line.endswith("\n"):
            line += "\n"
        try:
            with self.serial_lock:
                if self.ser:
                    self.ser.write(line.encode("ascii", errors="ignore"))
        except Exception:
            pass

    def serial_reader_loop(self):
        buf = b""
        while self.running:
            if not self.ser:
                time.sleep(1.0)
                self.open_serial()
                continue
            try:
                with self.serial_lock:
                    chunk = self.ser.read(256)
                if chunk:
                    buf += chunk
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        line = line.strip()
                        if not line:
                            continue
                        try:
                            data = json.loads(line.decode("ascii", errors="ignore"))
                            self.latest = data
                        except json.JSONDecodeError:
                            self.rx_errors += 1
                else:
                    time.sleep(0.05)
            except Exception:
                try:
                    if self.ser:
                        self.ser.close()
                except Exception:
                    pass
                self.ser = None
                time.sleep(1.0)

    # -----------------------------
    # UI builders
    # -----------------------------
    def make_value_grid(self, fields):
        grid = QGridLayout()
        labels = {}
        for r, f in enumerate(fields):
            grid.addWidget(QLabel(f.label), r, 0)
            v = QLabel("--")
            v.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
            grid.addWidget(v, r, 1)
            grid.addWidget(QLabel(f.units), r, 2)
            labels[f.key] = v
        return grid, labels

    def make_toggle(self, text_on, text_off, cmd_on, cmd_off):
        btn = QPushButton(text_off)
        btn.setCheckable(True)

        def _clicked(checked):
            btn.setText(text_on if checked else text_off)
            self.send(cmd_on if checked else cmd_off)

        btn.clicked.connect(_clicked)
        return btn

    def make_slider_pct(self, label_text, cmd_prefix):
        box = QGroupBox(label_text)
        lay = QHBoxLayout(box)
        slider = QSlider(Qt.Horizontal)
        slider.setRange(0, 100)
        slider.setValue(0)
        edit = QLineEdit("0")
        edit.setFixedWidth(60)
        btn = QPushButton("Send")
        val_lbl = QLabel("%")

        def _sync_from_slider(v):
            edit.setText(str(int(v)))

        def _send():
            try:
                v = float(edit.text().strip())
            except Exception:
                v = float(slider.value())
            v = max(0.0, min(100.0, v))
            slider.setValue(int(v))
            self.send(f"{cmd_prefix} {v:0.1f}")

        slider.valueChanged.connect(_sync_from_slider)
        btn.clicked.connect(_send)

        lay.addWidget(slider, stretch=1)
        lay.addWidget(edit)
        lay.addWidget(val_lbl)
        lay.addWidget(btn)
        return box, slider, edit

    def build_overview_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        # Status block
        g = QGroupBox("Status")
        grid = QGridLayout(g)
        self.ov_mode = QLabel("--")
        self.ov_timer = QLabel("--")
        self.ov_service = QLabel("--")
        grid.addWidget(QLabel("STAT_Mode"), 0, 0)
        grid.addWidget(self.ov_mode, 0, 1)
        grid.addWidget(QLabel("Mode Timer (s)"), 1, 0)
        grid.addWidget(self.ov_timer, 1, 1)
        grid.addWidget(QLabel("Service Mode"), 2, 0)
        grid.addWidget(self.ov_service, 2, 1)

        lay.addWidget(g)

        # Quick setpoints
        sp = QGroupBox("Setpoints (Apply)")
        sp_lay = QGridLayout(sp)
        self.ed_sp_lwt = QLineEdit("44.0")
        self.ed_sp_db = QLineEdit("2.0")
        btn_apply = QPushButton("Apply SPs")
        btn_apply.clicked.connect(self.apply_setpoints)
        sp_lay.addWidget(QLabel("SP LWT (°F)"), 0, 0)
        sp_lay.addWidget(self.ed_sp_lwt, 0, 1)
        sp_lay.addWidget(QLabel("DB (°F)"), 1, 0)
        sp_lay.addWidget(self.ed_sp_db, 1, 1)
        sp_lay.addWidget(btn_apply, 2, 0, 1, 2)
        lay.addWidget(sp)

        # PVs
        pv = QGroupBox("Key Readings")
        pv_grid, self.ov_vals = self.make_value_grid([Field("EVAP_LWT_F","Evap LWT","°F"),
                                                     Field("F_EVAP_GPM","Evap Flow","GPM"),
                                                     Field("P_SUCTION","Suction P","psig"),
                                                     Field("P_COMP_DISCH","Disch P","psig")])
        pv.setLayout(pv_grid)
        lay.addWidget(pv)

        lay.addStretch(1)
        return w

    def build_evap_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        pv = QGroupBox("Evaporator Readings")
        pv_grid, self.evap_vals = self.make_value_grid([
            Field("EVAP_LWT_F","Evap LWT","°F"),
            Field("EVAP_EWT_F","Evap EWT","°F"),
            Field("F_EVAP_GPM","Evap Flow","GPM"),
        ])
        pv.setLayout(pv_grid)
        lay.addWidget(pv)

        ctrl = QGroupBox("Evaporator Controls (SERVICE mode)")
        c = QVBoxLayout(ctrl)
        self.btn_evap_pump = self.make_toggle("Evap Pump ON", "Evap Pump OFF", "PUMP EVAP ON", "PUMP EVAP OFF")
        c.addWidget(self.btn_evap_pump)

        box, self.sl_evap_eev, _ = self.make_slider_pct("Evap EEV Position", "EEV EVAP")
        c.addWidget(box)
        lay.addWidget(ctrl)

        lay.addStretch(1)
        return w

    def build_cond_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        pv = QGroupBox("Condenser Readings")
        pv_grid, self.cond_vals = self.make_value_grid([
            Field("COND_LWT_F","Cond LWT","°F"),
            Field("COND_EWT_F","Cond EWT","°F"),
            Field("F_COND_GPM","Cond Flow","GPM"),
        ])
        pv.setLayout(pv_grid)
        lay.addWidget(pv)

        ctrl = QGroupBox("Condenser Controls (SERVICE mode)")
        c = QVBoxLayout(ctrl)
        self.btn_cond_pump = self.make_toggle("Cond Pump ON", "Cond Pump OFF", "PUMP COND ON", "PUMP COND OFF")
        c.addWidget(self.btn_cond_pump)

        box, self.sl_econ_eev, _ = self.make_slider_pct("Economizer EEV Position", "EEV ECON")
        c.addWidget(box)
        lay.addWidget(ctrl)

        lay.addStretch(1)
        return w

    def build_refrig_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        pv = QGroupBox("Refrigerant Readings")
        pv_grid, self.ref_vals = self.make_value_grid([
            Field("P_SUCTION","Suction P","psig"),
            Field("P_COMP_DISCH","Disch P","psig"),
            Field("P_LIQUID_LINE","Liquid Line P","psig"),
            Field("SUCTION_F","Suction Temp","°F"),
            Field("DISCHARGE_F","Discharge Temp","°F"),
            Field("LIQ_LINE_F","Liquid Temp","°F"),
        ])
        pv.setLayout(pv_grid)
        lay.addWidget(pv)

        lay.addStretch(1)
        return w

    def build_tower_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        pv = QGroupBox("Cooling Tower Readings")
        pv_grid, self.tower_vals = self.make_value_grid([
            Field("F_COND_GPM","Cond Flow","GPM"),
            Field("COND_EWT_F","Cond EWT","°F"),
        ])
        pv.setLayout(pv_grid)
        lay.addWidget(pv)

        ctrl = QGroupBox("Cooling Tower Controls (SERVICE mode)")
        c = QVBoxLayout(ctrl)
        self.btn_ct_pump = self.make_toggle("CT Pump ON", "CT Pump OFF", "PUMP CT ON", "PUMP CT OFF")
        c.addWidget(self.btn_ct_pump)

        box, self.sl_tower_fan, _ = self.make_slider_pct("Tower Fan Speed", "TOWER FAN")
        c.addWidget(box)
        lay.addWidget(ctrl)

        lay.addStretch(1)
        return w

    def build_vfd_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        pv = QGroupBox("Compressor / VFD Readings")
        pv_grid, self.vfd_vals = self.make_value_grid([
            Field("AO_CompSpeedCmd_pct","Speed Cmd","%"),
            Field("MAN_VFDRun","Manual Run",""),
            Field("MAN_VFDSpeedPct","Manual Speed","%"),
        ])
        pv.setLayout(pv_grid)
        lay.addWidget(pv)

        ctrl = QGroupBox("Compressor / VFD Controls (SERVICE mode)")
        c = QVBoxLayout(ctrl)
        self.btn_vfd_run = self.make_toggle("VFD RUN ON", "VFD RUN OFF", "VFD RUN ON", "VFD RUN OFF")
        c.addWidget(self.btn_vfd_run)

        box, self.sl_vfd_speed, _ = self.make_slider_pct("VFD Speed Command", "VFD SPEED")
        c.addWidget(box)

        lay.addWidget(ctrl)
        lay.addStretch(1)
        return w

    def build_alarms_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        a = QGroupBox("Active Alarms")
        grid = QGridLayout(a)
        self.alarm_labels = {}
        for r, f in enumerate(ALARM_FIELDS):
            grid.addWidget(QLabel(f.label), r, 0)
            v = QLabel("--")
            v.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
            grid.addWidget(v, r, 1)
            self.alarm_labels[f.key] = v
        lay.addWidget(a)

        ev = QGroupBox("Last Event")
        ev_lay = QVBoxLayout(ev)
        self.txt_event = QTextEdit()
        self.txt_event.setReadOnly(True)
        ev_lay.addWidget(self.txt_event)
        lay.addWidget(ev)

        lay.addStretch(1)
        return w

    def build_training_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        ctrl = QGroupBox("Training / Simulation Controls")
        g = QGridLayout(ctrl)

        self.btn_sim_on = QPushButton("SIM ON")
        self.btn_sim_off = QPushButton("SIM OFF")
        self.btn_sim_on.clicked.connect(lambda: self.send("SIM ON"))
        self.btn_sim_off.clicked.connect(lambda: self.send("SIM OFF"))

        self.cmb_fault = QComboBox()
        self.cmb_fault.addItems([
            "NONE",
            "EVAP_FLOW_FAIL",
            "EVAP_FLOW_LOST",
            "COND_FLOW_FAIL",
            "COND_FLOW_LOST",
            "HI_DISCH_P",
            "LO_SUCTION_P",
            "SENSOR_STUCK_LWT",
        ])
        self.btn_fault = QPushButton("Apply Fault")
        self.btn_fault.clicked.connect(self.apply_fault)

        g.addWidget(self.btn_sim_on, 0, 0)
        g.addWidget(self.btn_sim_off, 0, 1)
        g.addWidget(QLabel("Fault:"), 1, 0)
        g.addWidget(self.cmb_fault, 1, 1)
        g.addWidget(self.btn_fault, 2, 0, 1, 2)

        lay.addWidget(ctrl)
        lay.addStretch(1)
        return w

    def build_calibration_tab(self):
        w = QWidget()
        lay = QVBoxLayout(w)

        info = QLabel("Calibration applies offsets in engineering units.\n"
                      "Examples:\n"
                      "  CAL TEMP EVAP_LWT  +0.5\n"
                      "  CAL PRESS P_SUCTION -1.0\n"
                      "  CAL FLOW EVAP +0.25\n")
        lay.addWidget(info)

        box = QGroupBox("Quick Cal (common points)")
        g = QGridLayout(box)

        self.ed_lwt_ofs = QLineEdit("0.0")
        self.btn_lwt_ofs = QPushButton("Apply EVAP_LWT offset")
        self.btn_lwt_ofs.clicked.connect(self.apply_lwt_offset)

        self.ed_evap_flow_ofs = QLineEdit("0.0")
        self.btn_evap_flow = QPushButton("Apply EVAP flow offset")
        self.btn_evap_flow.clicked.connect(lambda: self.apply_flow_offset("EVAP", self.ed_evap_flow_ofs))

        self.ed_cond_flow_ofs = QLineEdit("0.0")
        self.btn_cond_flow = QPushButton("Apply COND flow offset")
        self.btn_cond_flow.clicked.connect(lambda: self.apply_flow_offset("COND", self.ed_cond_flow_ofs))

        g.addWidget(QLabel("EVAP_LWT offset (°F)"), 0, 0)
        g.addWidget(self.ed_lwt_ofs, 0, 1)
        g.addWidget(self.btn_lwt_ofs, 0, 2)

        g.addWidget(QLabel("EVAP flow offset (GPM)"), 1, 0)
        g.addWidget(self.ed_evap_flow_ofs, 1, 1)
        g.addWidget(self.btn_evap_flow, 1, 2)

        g.addWidget(QLabel("COND flow offset (GPM)"), 2, 0)
        g.addWidget(self.ed_cond_flow_ofs, 2, 1)
        g.addWidget(self.btn_cond_flow, 2, 2)

        lay.addWidget(box)
        lay.addStretch(1)
        return w

    # -----------------------------
    # Apply actions
    # -----------------------------
    def apply_mode(self):
        m = self.cmb_mode.currentText().strip().upper()
        self.send(f"MODE {m}")

    def apply_setpoints(self):
        try:
            sp = float(self.ed_sp_lwt.text().strip())
        except Exception:
            sp = 44.0
        try:
            db = float(self.ed_sp_db.text().strip())
        except Exception:
            db = 2.0
        self.send(f"SP LWT {sp:0.2f}")
        self.send(f"SP LWT_DB {db:0.2f}")

    def apply_fault(self):
        f = self.cmb_fault.currentText().strip()
        if f == "NONE":
            self.send("SIM OFF")
        else:
            self.send("SIM FAULT " + f)

    def apply_lwt_offset(self):
        try:
            v = float(self.ed_lwt_ofs.text().strip())
        except Exception:
            v = 0.0
        self.send(f"CAL TEMP EVAP_LWT {v:0.3f}")

    def apply_flow_offset(self, which, edit):
        try:
            v = float(edit.text().strip())
        except Exception:
            v = 0.0
        self.send(f"CAL FLOW {which} {v:0.3f}")

    # -----------------------------
    # Refresh
    # -----------------------------
    def refresh(self):
        data = self.latest or {}

        self.lbl_status.setText("Connected" if self.ser else "Disconnected")

        # Overview
        self.ov_mode.setText(str(data.get("STAT_Mode", "--")))
        self.ov_timer.setText(fmt(data.get("STAT_ModeTimer_s", None), 1))
        self.ov_service.setText("ON" if data.get("STAT_ServiceMode", False) else "OFF")

        # Setpoint display (reflect controller values)
        if "SP_LWT" in data:
            self.ed_sp_lwt.setText(fmt(data.get("SP_LWT"), 1))
        if "SP_LWT_DB" in data:
            self.ed_sp_db.setText(fmt(data.get("SP_LWT_DB"), 1))

        # Value labels
        for key, lbl in getattr(self, "ov_vals", {}).items():
            lbl.setText(fmt(data.get(key), 2 if "GPM" in key else 1))
        for key, lbl in getattr(self, "evap_vals", {}).items():
            lbl.setText(fmt(data.get(key), 2 if "GPM" in key else 1))
        for key, lbl in getattr(self, "cond_vals", {}).items():
            lbl.setText(fmt(data.get(key), 2 if "GPM" in key else 1))
        for key, lbl in getattr(self, "ref_vals", {}).items():
            lbl.setText(fmt(data.get(key), 1))
        for key, lbl in getattr(self, "tower_vals", {}).items():
            lbl.setText(fmt(data.get(key), 2 if "GPM" in key else 1))
        for key, lbl in getattr(self, "vfd_vals", {}).items():
            lbl.setText(fmt(data.get(key), 1))

        # Alarms
        for k, lbl in getattr(self, "alarm_labels", {}).items():
            lbl.setText("ON" if data.get(k, False) else "OFF")

        # Event log
        last_evt = data.get("EVT_Last", "")
        if last_evt:
            self.txt_event.setPlainText(last_evt)

    def closeEvent(self, event):
        self.running = False
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
        super().closeEvent(event)

def main():
    app = QApplication(sys.argv)
    w = ChillerHMI()
    w.resize(1100, 650)
    w.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
