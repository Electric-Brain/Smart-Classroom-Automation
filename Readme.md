# 🏫 Autovent — A Complete Smart Classroom

> An IoT-based smart classroom automation system developed as part of the TCET Inhouse Internship in collaboration with **Agastya International Foundation**, deployed and demonstrated at **Sanmitra Mandal Vidhyalaya, Goregaon East**.
>
> 🏆 **Winner — Best Depth of Understanding Award** at the project showcase held at Yashwantrao Chavan Center, Mumbai.

---

## 📌 Table of Contents

- [Project Overview](#project-overview)
- [System Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Pin Mapping — All Boards](#pin-mapping--all-boards)
- [Firmware Modules](#firmware-modules)
- [UART Communication Protocol](#uart-communication-protocol)
- [Web Dashboard](#web-dashboard)
- [Setup & Flashing](#setup--flashing)
- [Libraries Required](#libraries-required)
- [Known Issues & Fixes Applied](#known-issues--fixes-applied)
- [Team](#team)

---

## Project Overview

Autovent is a fully integrated smart classroom system that automates the physical environment of a classroom — windows, curtains, door, lights, and fan — based on sensor inputs, real-time monitoring via a web dashboard, and a touch/manual override. The system was conceptualized by school students to eliminate the manual workload placed on the school peon, who was responsible for opening and closing windows, doors, and curtains multiple times a day.

**Core capabilities:**

- Automated door opening via PIR motion detection with a 5-second auto-close timer
- Motor-controlled windows (×2), curtains, and door (servo + DC motor)
- Relay-controlled lights and fan
- Live MJPEG video stream from an ESP32-CAM CCTV module
- Real-time temperature and humidity monitoring via DHT22
- Reed switch confirmation for window close verification
- Stuck-window detection with alert and force-close override
- Full web dashboard with WebSocket-based live state sync
- TFT display on the main controller showing system status
- Touch sensor for physical master on/off toggle


---

## System Architecture

```
                        ┌─────────────────────────────────┐
                        │      ESP32 #1 — Main Controller  │
                        │  (DHT22, Relays, PIR, Touch,     │
                        │   TFT Display, Reed Switches,    │
                        │   WiFi Web Server, WebSocket)    │
                        └────────────┬────────────────────┘
                                     │ UART2 (TX:17 / RX:16)
                                     │ 115200 baud
                        ┌────────────▼────────────────────┐
                        │      ESP32 #2 — Motor Controller │
                        │  (Servo: Door, Win1)             │
                        │  (DC Motor: Win2, Curtain)       │
                        │  (TB6612 / L298 driver)          │
                        └─────────────────────────────────┘

                        ┌─────────────────────────────────┐
                        │    ESP32-CAM — CCTV Module       │
                        │  (AI Thinker, OV2640 sensor)     │
                        │  MJPEG stream :81/stream         │
                        │  Flash LED GPIO4                 │
                        └─────────────────────────────────┘

                        ┌─────────────────────────────────┐
                        │  ESP32 + TFT — BookBuddy Display │
                        │  (ILI9341/ST7789 TFT via SPI,   │
                        │   DFPlayer Mini audio module)    │
                        └─────────────────────────────────┘

                              [ Browser / Phone ]
                                      │
                              HTTP + WebSocket
                                      │
                              ESP32 #1 Web Server
```

---

## Hardware Components

### ESP32 #1 — Main Controller

| Component | Model / Spec | Notes |
|-----------|-------------|-------|
| Microcontroller | ESP32 DevKit v1 (30-pin) | Primary controller, WiFi host |
| TFT Display | ILI9488, 480×320, SPI | Status display, VSPI bus |
| Temperature & Humidity Sensor | DHT22 (AM2302) | 3.3V, data on GPIO26 |
| Relay Module | 2-channel, 5V, active-LOW, opto-isolated | Controls light & fan |
| PIR Sensor | HC-SR501 | Motion detection, door auto-open |
| Touch Sensor | TTP223 capacitive module | Master ON/OFF toggle |
| Buzzer | Active buzzer, 3.3–5V | Audio feedback |
| Reed Switch ×2 | Normally-open, magnetic | Window close confirmation |

### ESP32 #2 — Motor Controller

| Component | Model / Spec | Notes |
|-----------|-------------|-------|
| Microcontroller | ESP32 DevKit v1 (30-pin) | Receives UART commands |
| Servo Motor — Door | Standard 180° servo | Anticlockwise direction (pulse reversed) |
| Servo Motor — Window 1 | Standard 180° servo | Normal direction |
| DC Motor — Window 2 | 5–12V DC geared motor | Via H-bridge driver |
| DC Motor — Curtain | 5–12V DC geared motor | Via H-bridge driver |
| Motor Driver | TB6612FNG or L298N | Dual H-bridge for WIN2 + Curtain |

### ESP32-CAM — CCTV Module

| Component | Model / Spec | Notes |
|-----------|-------------|-------|
| Camera Module | AI Thinker ESP32-CAM | OV2640 sensor, onboard PSRAM |
| Flash LED | White LED on GPIO4 | PWM-controlled intensity (0–255) |
| Antenna | Onboard PCB antenna | External antenna recommended for range |



---

## Pin Mapping — All Boards

### ESP32 #1 — Main Controller

| Signal | GPIO | Direction | Notes |
|--------|------|-----------|-------|
| TFT CS | 15 | OUT | VSPI chip select |
| TFT DC | 2 | OUT | Data/Command |
| TFT RST | 4 | OUT | Reset |
| TFT MOSI | 23 | OUT | VSPI MOSI |
| TFT MISO | 19 | IN | VSPI MISO |
| TFT SCK | 18 | OUT | VSPI clock |
| DHT22 DATA | 26 | IN | 10kΩ pull-up recommended |
| Relay LIGHT | 32 | OUT | LOW = relay ON (active-LOW) |
| Relay FAN | 33 | OUT | LOW = relay ON (active-LOW) |
| Buzzer | 5 | OUT | HIGH = buzzer ON |
| Touch TTP223 | 34 | IN | Input-only, HIGH = touched |
| PIR HC-SR501 | 35 | IN | Input-only, HIGH = motion |
| UART2 TX → ESP32#2 RX | 17 | OUT | 115200 baud |
| UART2 RX ← ESP32#2 TX | 16 | IN | 115200 baud |
| Reed Switch 1 | 36 | IN | Input-only, external 10kΩ pull-up to 3.3V required |
| Reed Switch 2 | 39 | IN | Input-only, external 10kΩ pull-up to 3.3V required |

> ⚠️ **GPIO 34, 35, 36, 39 are input-only on ESP32.** No internal pull-ups are available for 36 and 39. Use external 10kΩ resistors to 3.3V for the reed switches.

### ESP32 #2 — Motor Controller

| Signal | GPIO | Direction | Notes |
|--------|------|-----------|-------|
| Servo Door | 5 | OUT | Reversed pulse (2400µs–500µs) for anticlockwise |
| Servo Win1 | 18 | OUT | Normal pulse (500µs–2400µs) |
| Motor Driver STBY | 23 | OUT | HIGH = driver enabled |
| Motor A PWM (Curtain) | 19 | OUT | LEDC channel, 18kHz, 10-bit |
| Motor A IN1 (Curtain) | 21 | OUT | Direction |
| Motor A IN2 (Curtain) | 22 | OUT | Direction |
| Motor B PWM (Win2) | 25 | OUT | LEDC channel, 18kHz, 10-bit |
| Motor B IN1 (Win2) | 26 | OUT | Direction |
| Motor B IN2 (Win2) | 27 | OUT | Direction |
| UART2 RX ← ESP32#1 TX | 16 | IN | 115200 baud |
| UART2 TX → ESP32#1 RX | 17 | OUT | 115200 baud |

### ESP32-CAM — AI Thinker

| Signal | GPIO | Notes |
|--------|------|-------|
| PWDN | 32 | Camera power down |
| XCLK | 0 | 20 MHz clock (LEDC ch0) |
| SIOD (SDA) | 26 | SCCB/I2C data |
| SIOC (SCL) | 27 | SCCB/I2C clock |
| D7–D0 | 35,34,39,36,21,19,18,5 | Parallel data bus |
| VSYNC | 25 | Frame sync |
| HREF | 23 | Line sync |
| PCLK | 22 | Pixel clock |
| Flash LED | 4 | PWM, init AFTER camera init |

> ⚠️ **GPIO4 is shared** between the flash LED and the SD card interface on AI Thinker boards. The flash LED must be initialized **after** `esp_camera_init()` to avoid LEDC conflicts.



## Firmware Modules

### `ESP32_Main.ino` (ESP32 #1)

The central controller. Responsibilities:
- Hosts an **AsyncWebServer** on port 80 with WebSocket endpoint `/ws`
- Serves the full web dashboard (HTML inlined in `web_ui_v7.h`)
- Polls DHT22 every 3 seconds and pushes updates via WebSocket
- Polls PIR every loop — rising edge opens door via UART, starts 5-second auto-close timer
- Polls TTP223 touch sensor with 50ms debounce for master on/off
- Polls reed switches with 5-sample debounce for window confirmation
- Forwards motor commands to ESP32 #2 over UART2 (with inversion flags for reversed motors)
- Updates TFT display (double-buffered sprite) every 1 second
- Exposes REST endpoints: `/api/state`, `/api/cam`

**Motor inversion flags** (in `ESP32_Main.ino`):
```cpp
#define INV_WIN1    true    // Window 1 open/close are physically swapped
#define INV_WIN2    false
#define INV_CURTAIN false
#define INV_DOOR    true    // Door open/close are physically swapped
```
Set to `true` if the motor rotates in the wrong direction without rewiring.

---

### `Motor_v3.ino` (ESP32 #2)

Dedicated motor controller. Listens on UART2 at 115200 baud.

**Servo configuration (anticlockwise door):**
```cpp
// REVERSED pulse range causes anticlockwise rotation:
servoDoor.attach(SERVO_DOOR_PIN, 2400, 500);
// Normal pulse range for Win1:
servoWin1.attach(SERVO_WIN1_PIN, 500, 2400);
```

DC motors use a 3-second timeout (`MOTOR_TIMEOUT = 3000ms`) after which `motorStop()` is called automatically. This prevents stall damage and generates a `DONE:<name>` response on UART.

---

### `CAM_v9.ino` (ESP32-CAM)

Standalone camera firmware. Exposes three HTTP endpoints:

| Endpoint | Port | Description |
|----------|------|-------------|
| `/stream` | 81 | MJPEG live stream (~12fps) |
| `/capture` | 80 | Single JPEG snapshot |
| `/flash?intensity=N` | 80 | Set flash LED brightness (0–255) |

Key fixes implemented:
- `fb_count = 1` + `CAMERA_GRAB_LATEST` prevents frame desync/freeze
- Frame pacing at 80ms intervals prevents WiFi buffer overflow
- Flash LED initialized **after** `esp_camera_init()` (GPIO4 LEDC conflict fix)
- Indoor sensor tuning: auto white balance (mode 2), AEC2, gain control enabled

---



### `web_ui_v7.h`

The complete web dashboard, inlined as a PROGMEM string in the main firmware. Built with vanilla HTML/CSS/JS — no external frameworks needed at runtime (only a Google Fonts CDN call for typography).

Features:
- Dark-glass design, responsive grid layout
- Live temperature and humidity
- Per-actuator open/close buttons (Window 1, Window 2, Curtain, Door)
- Master Activate / Deactivate button
- Reed switch status bars with stuck-window alerts and force-close button
- PIR countdown timer (5-second door auto-close visible in UI)
- MJPEG camera feed with flash toggle and stream reload
- WebSocket reconnection with auto-retry

---

## UART Communication Protocol

All commands are ASCII strings terminated with `\n`. Communication is between ESP32 #1 (master) and ESP32 #2 (motor controller).

### Commands — ESP32 #1 → ESP32 #2

| Command | Action |
|---------|--------|
| `WIN1_OPEN` | Open Window 1 (servo) |
| `WIN1_CLOSE` | Close Window 1 (servo) |
| `WIN2_OPEN` | Open Window 2 (DC motor, forward) |
| `WIN2_CLOSE` | Close Window 2 (DC motor, reverse) |
| `WIN1_FORCE_CLOSE` | Force close WIN1 (used on stuck alert) |
| `WIN2_FORCE_CLOSE` | Force close WIN2 (used on stuck alert) |
| `CURTAIN_OPEN` | Open curtain (DC motor, forward) |
| `CURTAIN_CLOSE` | Close curtain (DC motor, reverse) |
| `DOOR_OPEN` | Open door (servo, 0° → 180°) |
| `DOOR_CLOSE` | Close door (servo, 180° → 0°) |
| `ALL_STOP` | Stop all DC motors, home all servos |

> ⚠️ Commands are auto-inverted by ESP32 #1 for channels flagged in `INV_WIN1` / `INV_DOOR`. The `state.*` variables always reflect the logical intent; only the UART wire carries the inverted string.

### Responses — ESP32 #2 → ESP32 #1

| Response | Meaning |
|----------|---------|
| `OK` | Command acknowledged |
| `DONE:WIN2` | Win2 DC motor stopped (timeout reached) |
| `DONE:CURTAIN` | Curtain DC motor stopped (timeout reached) |
| `ALERT:WIN1_STUCK` | Win1 reed not triggered after close attempt |
| `ALERT:WIN2_STUCK` | Win2 reed not triggered after close attempt |
| `WIN1_OPENED` | Win1 reed switch triggered (opened confirmed) |
| `WIN2_OPENED` | Win2 reed switch triggered (opened confirmed) |
| `READY` | ESP32 #2 boot complete |
| `ERR:UNKNOWN` | Unrecognized command received |

---

## Web Dashboard

Access the dashboard by navigating to the IP address shown on the TFT display after boot (e.g. `http://192.168.x.x`).

The dashboard communicates over WebSocket (`ws://<ip>/ws`). State is pushed from the ESP32 every 2 seconds (heartbeat) and also on any event change.

**REST API endpoints:**

| Endpoint | Method | Response |
|----------|--------|----------|
| `/` | GET | Full dashboard HTML |
| `/api/state` | GET | JSON snapshot of all sensor and actuator states |
| `/api/cam` | GET | JSON with stream URL and flash URL |

---

## Setup & Flashing

### Prerequisites

- Arduino IDE 2.x or PlatformIO
- ESP32 board package installed (`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`)
- All libraries listed below

### Steps

1. **Configure WiFi credentials** in `ESP32_Main.ino` and `CAM_v9.ino`:
   ```cpp
   const char* WIFI_SSID     = "your_ssid";
   const char* WIFI_PASSWORD = "your_password";
   ```

2. **Update CAM IP** in `ESP32_Main.ino` after first flashing the CAM:
   ```cpp
   const char* CAM_STREAM_URL = "http://<cam_ip>:81/stream";
   const char* CAM_FLASH_URL  = "http://<cam_ip>/flash";
   ```

3. **Configure TFT_eSPI** — edit `User_Setup.h` in the TFT_eSPI library to match the pin wiring for your display driver (ILI9488 for the main controller TFT).

4. **Flash order**:
   - Flash ESP32-CAM first → note down IP from Serial Monitor
   - Flash ESP32 #2 (motor controller)
   - Flash ESP32 #1 (main controller) with correct CAM IP

5. **Motor inversion** — if any motor moves in the wrong direction, toggle the corresponding `INV_*` flag in `ESP32_Main.ino` (no hardware rewiring needed).

---

## Libraries Required

### ESP32 #1 (Main Controller)

| Library | Source |
|---------|--------|
| ESPAsyncWebServer | https://github.com/me-no-dev/ESPAsyncWebServer |
| AsyncTCP | https://github.com/me-no-dev/AsyncTCP |
| ArduinoJson | Arduino Library Manager |
| DHT sensor library | Adafruit, Arduino Library Manager |
| TFT_eSPI | Arduino Library Manager |

### ESP32 #2 (Motor Controller)

| Library | Source |
|---------|--------|
| ESP32Servo | Arduino Library Manager |

### ESP32-CAM

| Library | Source |
|---------|--------|
| esp_camera | Built into ESP32 Arduino core |
| esp_http_server | Built into ESP32 Arduino core |


---

## Known Issues & Fixes Applied

| Issue | Root Cause | Fix Applied |
|-------|-----------|-------------|
| Door stays open after reboot | State initialized as `true` | `state.doorOpen = false` on boot; `DOOR_CLOSE` sent in `setup()` |
| PIR door never auto-closes | `pirDoorOpenedAt` was reset every loop iteration while PIR was HIGH | Timer reset only on rising edge (new motion event) |
| Master OFF does not close door | Door was excluded from `applyMasterSwitch(false)` | Door CLOSE added to both ON and OFF paths; PIR timer cancelled before sending close |
| ESP32-CAM stream freezes | `fb_count=2` caused frame buffer desync | Set `fb_count=1` + `CAMERA_GRAB_LATEST` |
| Flash LED non-functional | GPIO4 LEDC conflict with camera init | `ledcAttach(FLASH_LED_PIN, ...)` moved to after `esp_camera_init()` |
| UART motor commands inverted | Physical motor wiring reversed for WIN1 and DOOR | Software inversion flags `INV_WIN1=true`, `INV_DOOR=true` in `sendMotorCmd()` |
| ESP32-CAM error 112 (port conflict) | Two `httpd_start()` calls with same port | Stream server fixed at port 81, control server at port 80, with separate `ctrl_port` values |
| Multiple UART commands dropped | ESP32 #2 not finishing processing before next command arrives | 300ms gap (`MOTOR_CMD_GAP_MS`) inserted between sequential UART sends in master switch |

---

## Team

| Name | Role |
|------|------|
| Gautam | Firmware development, hardware integration, system architecture |
| Shivangi | Co-developer, project co-lead |

**Institution:** Thakur College of Engineering and Technology (TCET), Mumbai
**Program:** TCET Inhouse Internship
**Collaboration:** Agastya International Foundation
**Deployment Site:** Sanmitra Mandal Vidhyalaya, Goregaon East, Mumbai

---

## License

This project is developed for academic and educational purposes under the TCET Inhouse Internship program. Please contact the authors before reuse or redistribution.
