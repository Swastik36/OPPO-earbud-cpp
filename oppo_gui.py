#!/usr/bin/env python3
import sys
import os
import subprocess
import threading
from PySide6.QtCore import QObject, Slot, Signal, Property
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine

CPP_CLI_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "oppoctl-cpp")

class EarbudController(QObject):
    connectedChanged = Signal()
    batteryChanged = Signal()
    eqChanged = Signal()
    gameModeChanged = Signal()
    statusTextChanged = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._connected = False
        self._leftBattery = 0
        self._leftCharging = False
        self._rightBattery = 0
        self._rightCharging = False
        self._caseBattery = -1
        self._caseCharging = False
        self._eq = 0
        self._gameMode = False
        self._statusText = "Disconnected"
        self._mac = "60:55:56:22:49:A0"

    @Property(bool, notify=connectedChanged)
    def isConnected(self):
        return self._connected

    @Property(int, notify=batteryChanged)
    def leftBattery(self):
        return self._leftBattery

    @Property(bool, notify=batteryChanged)
    def leftCharging(self):
        return self._leftCharging

    @Property(int, notify=batteryChanged)
    def rightBattery(self):
        return self._rightBattery

    @Property(bool, notify=batteryChanged)
    def rightCharging(self):
        return self._rightCharging

    @Property(int, notify=batteryChanged)
    def caseBattery(self):
        return self._caseBattery

    @Property(bool, notify=batteryChanged)
    def caseCharging(self):
        return self._caseCharging

    @Property(int, notify=eqChanged)
    def currentEQ(self):
        return self._eq

    @Property(bool, notify=gameModeChanged)
    def gameMode(self):
        return self._gameMode

    @Property(str, notify=statusTextChanged)
    def statusText(self):
        return self._statusText

    def _run_cpp_cmd(self, args):
        cmd = [CPP_CLI_PATH, "--mac", self._mac] + args
        try:
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            return res.stdout.strip()
        except Exception as e:
            return f"Error: {e}"

    @Slot(str)
    def connectDevice(self, mac="60:55:56:22:49:A0"):
        self._mac = mac
        self._statusText = f"Connecting to {mac}..."
        self.statusTextChanged.emit()

        def do_connect():
            output = self._run_cpp_cmd(["battery"])
            if "Battery" in output or "Left:" in output or "Error" not in output:
                self._connected = True
                self._statusText = "Connected to C++ Protocol Engine"
                self._parse_battery(output)
                self.connectedChanged.emit()
                self.statusTextChanged.emit()
                self._fetch_eq()
                self._fetch_gamemode()
            else:
                self._connected = False
                self._statusText = f"Connection Failed: {output}"
                self.connectedChanged.emit()
                self.statusTextChanged.emit()

        threading.Thread(target=do_connect, daemon=True).start()

    @Slot()
    def disconnectDevice(self):
        self._connected = False
        self._statusText = "Disconnected"
        self.connectedChanged.emit()
        self.statusTextChanged.emit()

    def _parse_battery(self, text):
        try:
            if "Left:" in text:
                parts = text.split(",")
                for p in parts:
                    if "Left:" in p:
                        val = int(p.split("%")[0].split("Left:")[1].strip())
                        chg = "Charging=Yes" in p
                        self._leftBattery = val
                        self._leftCharging = chg
                    elif "Right:" in p:
                        val = int(p.split("%")[0].split("Right:")[1].strip())
                        chg = "Charging=Yes" in p
                        self._rightBattery = val
                        self._rightCharging = chg
                    elif "Case:" in p:
                        val = int(p.split("%")[0].split("Case:")[1].strip())
                        chg = "Charging=Yes" in p
                        self._caseBattery = val
                        self._caseCharging = chg
                self.batteryChanged.emit()
        except Exception as e:
            print("Battery parse error:", e)

    def _fetch_eq(self):
        def task():
            out = self._run_cpp_cmd(["eq"])
            if "Preset:" in out:
                if "Original" in out: self._eq = 0
                elif "Clear Vocals" in out: self._eq = 1
                elif "Bass Boost" in out: self._eq = 2
                self.eqChanged.emit()
        threading.Thread(target=task, daemon=True).start()

    def _fetch_gamemode(self):
        def task():
            out = self._run_cpp_cmd(["gamemode"])
            if "Enabled" in out:
                self._gameMode = True
                self.gameModeChanged.emit()
            elif "Disabled" in out:
                self._gameMode = False
                self.gameModeChanged.emit()
        threading.Thread(target=task, daemon=True).start()

    @Slot(int)
    def setEQ(self, preset_index):
        names = {0: "original", 1: "vocals", 2: "bass"}
        preset_name = names.get(preset_index, "original")
        def task():
            self._run_cpp_cmd(["eq", preset_name])
            self._eq = preset_index
            self.eqChanged.emit()
        threading.Thread(target=task, daemon=True).start()

    @Slot(bool)
    def setGameMode(self, enable):
        mode_str = "on" if enable else "off"
        def task():
            self._run_cpp_cmd(["gamemode", mode_str])
            self._gameMode = enable
            self.gameModeChanged.emit()
        threading.Thread(target=task, daemon=True).start()

def main():
    app = QGuiApplication(sys.argv)

    controller = EarbudController()

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("controller", controller)

    qml_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "qml", "Main.qml")
    engine.load(qml_file)

    if not engine.rootObjects():
        sys.exit(-1)

    sys.exit(app.exec())

if __name__ == "__main__":
    main()
