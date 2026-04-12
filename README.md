# Library Monitoring System
### Smart IoT-Based Environment & Occupancy Monitor
**NMIMS STME — MPMC/COA Project**

---

## Project Structure

```
LibraryMonitoringSystem/
├── firmware/
│   └── library_monitor.ino      ← ESP32 Arduino code (upload this)
├── wiring/
│   └── wiring_diagram.html      ← Open in browser for full pin diagram
├── dashboard/
│   ├── index.html               ← Web dashboard (open directly in browser)
│   └── server.js                ← Optional Node.js bridge for real hardware
└── report/
    └── report.tex               ← LaTeX project report (compile with pdflatex)
```

---

## Quick Start

### 1. Flash ESP32

1. Install Arduino IDE 2.x
2. Add ESP32 board: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Install libraries: `DHT sensor library` (Adafruit), `Blynk`
4. Open `firmware/library_monitor.ino`
5. Fill in your credentials:
   ```cpp
   #define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
   #define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
6. Select board: `ESP32 Dev Module` → Upload

### 2. Blynk Setup

1. Create account at https://blynk.io
2. Create new Template → Device: ESP32
3. Add Datastreams:
   - V0: Double (Temperature)
   - V1: Double (Humidity)
   - V2: Double (Noise)
   - V3: Double (Air Quality)
   - V4: Integer (Occupancy)
   - V5: Integer (PIR)
   - V6: Integer (Comfort Score)
   - V7: Integer (Max Capacity) [writable]
   - V8: Integer (Buzzer) [writable]
   - V9: String (Status)
4. Copy Template ID and Auth Token into firmware

### 3. Web Dashboard

**Standalone (demo mode):** Just open `dashboard/index.html` in any browser. It runs on simulated data.

**With real hardware:** 
```bash
cd dashboard
npm install express cors
node server.js
# Open http://localhost:3000
```
The ESP32 can POST JSON directly to `http://<your-pc-ip>:3000/sensor`

### 4. Compile Report

```bash
cd report
pdflatex report.tex
pdflatex report.tex   # Run twice for TOC
```
Requires: LaTeX distribution (TeX Live / MiKTeX) with TikZ, booktabs, listings packages.

---

## Pin Wiring Summary

| Component        | ESP32 GPIO | Notes                          |
|------------------|-----------|--------------------------------|
| DHT22 DATA       | GPIO 4    | 4.7kΩ pull-up to 3.3V          |
| PIR OUT          | GPIO 14   | Adjust sensitivity trimmer     |
| Mic AO           | GPIO 34   | ADC input-only pin             |
| MQ-2 AO          | GPIO 35   | ADC input-only; preheat 24h    |
| HC-SR04 #1 TRIG  | GPIO 26   | Entry sensor                   |
| HC-SR04 #1 ECHO  | GPIO 27   | Voltage divider: 1kΩ + 2kΩ     |
| HC-SR04 #2 TRIG  | GPIO 32   | Exit sensor                    |
| HC-SR04 #2 ECHO  | GPIO 33   | Voltage divider: 1kΩ + 2kΩ     |
| LED Green        | GPIO 18   | 220Ω series resistor           |
| LED Red          | GPIO 19   | 220Ω series resistor           |
| Buzzer +         | GPIO 23   | NPN transistor recommended     |

---

## Grading Criteria Mapping

| Requirement                          | Implementation                        |
|--------------------------------------|---------------------------------------|
| ✅ Microcontroller of choice          | ESP32 DevKit v1                       |
| ✅ 5 physical parameters              | Temp, Humidity, Noise, Air, Proximity |
| ✅ 3 actuators                        | LED Green, LED Red, Buzzer            |
| ✅ WiFi module + internet             | ESP32 built-in, connected to Blynk    |
| ✅ Cloud logging                      | Blynk IoT (SuperChart history)        |
| ✅ Web/mobile app with real-time data | Custom dashboard + Blynk mobile app   |
| ✅ Control actuator via app           | Buzzer toggle on V8, capacity on V7   |
