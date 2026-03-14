# 🏫 Autovent — A Complete Smart Classroom

> An IoT-based smart classroom automation system developed as part of the TCET Inhouse Internship in collaboration with **Agastya International Foundation**, deployed and demonstrated at **Sanmitra Mandal Vidhyalaya, Goregaon East, Mumbai**.
>
> 🏆 **Winner — Best Depth of Understanding Award** at the project showcase held at Yashwantrao Chavan Center, Mumbai.

---

## 📌 Table of Contents

- [Project Overview](#project-overview)
- [System Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Power Architecture](#power-architecture)
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

Autovent is a fully integrated smart classroom automation system that automates the physical environment of a classroom — windows, curtains, door, lights, and fan — based on sensor inputs, real-time monitoring via a web dashboard, and a physical touch override. The system was conceptualized by school students to eliminate the manual workload placed on the school peon, who was responsible for opening and closing windows, doors, and curtains multiple times every school day.

**Core capabilities:**

- Automated door opening via PIR motion detection with a 5-second auto-close timer
- Motor-controlled windows (×2), curtains, and door (servo + DC motor)
- Relay-controlled lights and fan with PWM-based fan speed control
- Live MJPEG video surveillance stream from ESP32-CAM
- Real-time temperature and humidity monitoring via DHT22
- Audio announcements via DFPlayer Mini + 3W speaker
- Reed switch confirmation for window close verification with stuck-window alert
- Full web dashboard with WebSocket-based live state sync
- ILI9488 TFT display showing real-time system status
- Capacitive touch sensor for physical master ON/OFF toggle

---

## System Architecture

```
          ┌──────────────────────────────────────────┐
          │         ESP32 #1 — Main Controller        │
          │   TFT Display · PIR · Touch · Reed ×2     │
          │   WiFi Web Server · WebSocket Dashboard   │
          └───────┬─────────────────┬────────────────┘
                  │ UART2           │ UART2
                  │ TX:17 / RX:16   │ TX:17 / RX:16
         ┌────────▼──────────┐  ┌───▼────────────────────┐
         │    ESP32 #2       │  │      ESP32 #3           │
         │   Peripherals     │  │   Motor Controller      │
         │  Relay · DHT22    │  │  Servo ×2 · DC Motor ×2 │
         │  Buzzer (TXA)     │  │  TB6612 Driver          │
         │  DFPlayer + 3W    │  └─────────────────────────┘
         └───────────────────┘

          ┌──────────────────────────────────────────┐
          │         ESP32-CAM — CCTV Module           │
          │   AI Thinker · OV2640 · Flash LED GPIO4   │
          │   MJPEG :81/stream · Capture :80/capture  │
          └──────────────────────────────────────────┘

                      [ Browser / Phone ]
                              │
                      HTTP + WebSocket
                              │
                    ESP32 #1 Web Server (port 80)
```

---

## Hardware Components

### ESP32 #1 — Main Controller

| Component | Model / Spec | Notes |
|-----------|-------------|-------|
| Microcontroller | ESP32 DevKit v1 (30-pin) | Central brain, WiFi host, web server |
| TFT Display | ILI9488, 480×320, SPI | Live status display; VSPI bus |
| PIR Sensor | HC-SR501 | Motion detection → door auto-open |
| Touch Sensor | TTP223 capacitive module | Physical master ON/OFF toggle |
| Reed Switch ×2 | Normally-open, magnetic | Window close confirmation |

### ESP32 #2 — Peripherals Board

| Component | Model / Spec | Notes |
|-----------|-------------|-------|
| Microcontroller | ESP32 DevKit v1 (30-pin) | Receives UART commands from ESP32 #1 |
| Relay Module | 2-channel, 5V, active-LOW, opto-isolated | Channel 1: Lights · Channel 2: Fan |
| DHT22 Sensor | AM2302, 3.3V | Temperature & humidity; 10kΩ pull-up on data pin |
| Active Buzzer | 3.3–5V active buzzer | NPN transistor amplification circuit |
| DFPlayer Mini | MP3 audio module | Audio playback from microSD; UART-controlled |
| Speaker | 3W, 4–8Ω passive | Connected to DFPlayer Mini DAC output |

### ESP32 #3 — Motor Controller

| Component | Model / Spec | Notes |
|-----------|-------------|-------|
| Microcontroller | ESP32 DevKit v1 (30-pin) | Receives UART commands from ESP32 #1 |
| Servo Motor — Door | Standard 180° servo (MG996R or similar) | Anticlockwise rotation via reversed pulse range |
| Servo Motor — Window 1 | Standard 180° servo | Normal pulse direction |
| DC Geared Motor — Window 2 | 5–12V DC geared motor | Motor B channel on TB6612; 3s auto-timeout |
| DC Geared Motor — Curtain | 5–12V DC geared motor | Motor A channel on TB6612; 3s auto-timeout |
| TB6612FNG Motor Driver | Dual H-bridge module | Controls Win2 + Curtain DC motors |

### ESP32-CAM — CCTV Module

| Component | Model / Spec | Notes |
|-----------|-------------|-------|
| Camera Module | AI Thinker ESP32-CAM | OV2640 sensor, 2MP, onboard PSRAM |
| Flash LED | White LED on GPIO4 | PWM brightness 0–255 |
| Antenna | Onboard PCB antenna | External antenna recommended for better range |

### Power & Control Modules

| Component | Spec | Role |
|-----------|------|------|
| 5V DC Adapter | 5V, min 2A | Directly powers ESP32-CAM |
| 12V DC Adapter | 12V, min 3A | Main supply for all other boards + motors |
| LM2596 Buck Converter ×2 | Step-down, adjustable output | Steps 12V down for ESP32 logic and motor supply |
| PWM Motor Speed Controller | PWM module | Controls fan speed; between relay output and fan |

### Passive Components

| Component | Value | Purpose |
|-----------|-------|---------|
| Resistor | 10kΩ | Pull-up for Reed Switch GPIO 36 & 39; DHT22 data line |
| NPN Transistor | BC547 / 2N2222 | Buzzer amplification circuit on ESP32 #2 |
| Jumper Wires | M-M / M-F | Board interconnects |
| Breadboard / PCB | — | Prototyping and component mounting |

---

## Power Architecture

```
  ┌─────────────┐     Direct 5V      ┌──────────────────┐
  │  5V Adapter │ ─────────────────▶ │   ESP32-CAM      │
  └─────────────┘

  ┌──────────────┐    ┌──────────────────┐    ┌───────────────────────────────┐
  │  12V Adapter │──▶ │ LM2596 Buck #1   │──▶ │ ESP32 #1, #2, #3  (5V logic) │
  │              │    └──────────────────┘    └───────────────────────────────┘
  │              │    ┌──────────────────┐    ┌───────────────────────────────┐
  │              │──▶ │ LM2596 Buck #2   │──▶ │ DC Motors · Servos · Relays   │
  └──────────────┘    └──────────────────┘    └───────────────────────────────┘
```

> ⚠️ Always verify LM2596 output voltage with a multimeter before connecting any ESP32 board. Incorrect voltage will permanently damage the microcontroller.

---

## Pin Mapping — All Boards

### ESP32 #1 — Main Controller

| Signal | GPIO | Direction | Notes |
|--------|------|-----------|-------|
| TFT CS | 15 | OUT | VSPI chip select |
| TFT DC | 2 | OUT | Data/Command select |
| TFT RST | 4 | OUT | Display reset |
| TFT MOSI | 23 | OUT | VSPI MOSI |
| TFT MISO | 19 | IN | VSPI MISO |
| TFT SCK | 18 | OUT | VSPI clock |
| PIR HC-SR501 | 35 | IN | Input-only; HIGH = motion detected |
| Touch TTP223 | 34 | IN | Input-only; HIGH = touched |
| Reed Switch 1 | 36 | IN | Input-only; external 10kΩ pull-up to 3.3V required |
| Reed Switch 2 | 39 | IN | Input-only; external 10kΩ pull-up to 3.3V required |
| UART2 TX → ESP32 #2/#3 RX | 17 | OUT | 115200 baud, 8N1 |
| UART2 RX ← ESP32 #2/#3 TX | 16 | IN | 115200 baud, 8N1 |

> ⚠️ GPIO 34, 35, 36, 39 are input-only. GPIO 36 and 39 have no internal pull-up — external 10kΩ resistors to 3.3V are mandatory.

### ESP32 #2 — Peripherals Board

| Signal | GPIO | Direction | Notes |
|--------|------|-----------|-------|
| Relay LIGHT | 32 | OUT | LOW = relay energised (active-LOW) |
| Relay FAN | 33 | OUT | LOW = relay energised (active-LOW) |
| DHT22 DATA | 26 | IN | 10kΩ pull-up to 3.3V |
| Buzzer (via NPN transistor) | 5 | OUT | HIGH = buzzer ON |
| DFPlayer Mini RX | 17 | OUT | HardwareSerial TX |
| DFPlayer Mini TX | 16 | IN | HardwareSerial RX |
| UART2 RX ← ESP32 #1 TX | 16 | IN | 115200 baud, 8N1 |
| UART2 TX → ESP32 #1 RX | 17 | OUT | 115200 baud, 8N1 |

### ESP32 #3 — Motor Controller

| Signal | GPIO | Direction | Notes |
|--------|------|-----------|-------|
| Servo Door | 5 | OUT | Reversed pulse: 2400µs (closed) → 500µs (open) |
| Servo Win1 | 18 | OUT | Normal pulse: 500µs (open) → 2400µs (closed) |
| Motor Driver STBY | 23 | OUT | HIGH = driver enabled; LOW = standby |
| Motor A PWM (Curtain) | 19 | OUT | LEDC, 18kHz, 10-bit |
| Motor A IN1 (Curtain) | 21 | OUT | H-bridge direction |
| Motor A IN2 (Curtain) | 22 | OUT | H-bridge direction |
| Motor B PWM (Win2) | 25 | OUT | LEDC, 18kHz, 10-bit |
| Motor B IN1 (Win2) | 26 | OUT | H-bridge direction |
| Motor B IN2 (Win2) | 27 | OUT | H-bridge direction |
| UART2 RX ← ESP32 #1 TX | 16 | IN | 115200 baud, 8N1 |
| UART2 TX → ESP32 #1 RX | 17 | OUT | 115200 baud, 8N1 |

### ESP32-CAM — AI Thinker

| Signal | GPIO | Notes |
|--------|------|-------|
| PWDN | 32 | Camera power down |
| XCLK | 0 | 20MHz clock (LEDC ch0) |
| SIOD (SDA) | 26 | SCCB/I2C data |
| SIOC (SCL) | 27 | SCCB/I2C clock |
| D7–D0 | 35,34,39,36,21,19,18,5 | 8-bit parallel pixel bus |
| VSYNC | 25 | Frame sync |
| HREF | 23 | Line sync |
| PCLK | 22 | Pixel clock |
| Flash LED | 4 | PWM — must init AFTER `esp_camera_init()` |

> ⚠️ GPIO4 is shared between flash LED and SD card on AI Thinker boards. Always initialise flash LED **after** `esp_camera_init()`.

---

## Firmware Modules

### `ESP32_Main.ino` — ESP32 #1

Central controller. Hosts **AsyncWebServer** on port 80 with WebSocket at `/ws`. Dashboard HTML served from `web_ui_v7.h` (PROGMEM).

- PIR: rising edge → UART `DOOR_OPEN` + 5s auto-close timer (resets only on new rising edge)
- Touch: 50ms debounce → toggles `applyMasterSwitch()`
- Reed switches: 5-sample debounce at 50ms → window close confirmation
- TFT: double-buffered sprite refresh every 1s
- REST: `/api/state` (full JSON), `/api/cam` (camera URLs)

**Motor inversion flags:**
```cpp
#define INV_WIN1    true    // Window 1 open/close physically swapped
#define INV_WIN2    false
#define INV_CURTAIN false
#define INV_DOOR    true    // Door open/close physically swapped
```

---

### `SmartClassroom_Peripheral.ino` — ESP32 #2

Handles all electrical peripherals. Receives UART commands from ESP32 #1.

- Relays: active-LOW; initialised HIGH (OFF) on boot
- DHT22: reads on `TEMP_REQ` command; responds with `TEMP:<val>,HUM:<val>`
- Buzzer: driven via NPN transistor for sufficient current; single beep and alert patterns
- DFPlayer Mini: plays MP3 from microSD on command; 3W speaker output

---

### `Motor_v3.ino` — ESP32 #3

Dedicated motor controller. Listens on UART2 at 115200 baud.

**Anticlockwise door servo (reversed pulse):**
```cpp
servoDoor.attach(SERVO_DOOR_PIN, 2400, 500);  // anticlockwise
servoWin1.attach(SERVO_WIN1_PIN, 500, 2400);  // normal
```

DC motors auto-stop after `MOTOR_TIMEOUT = 3000ms` and send `DONE:<n>` back to ESP32 #1.

---

### `CAM_v9.ino` — ESP32-CAM

Standalone camera node. Two HTTP servers on port 80 and 81:

| Endpoint | Port | Description |
|----------|------|-------------|
| `/stream` | 81 | MJPEG live stream ~12fps |
| `/capture` | 80 | Single JPEG snapshot |
| `/flash?intensity=N` | 80 | Flash LED brightness 0–255 |

Key fixes: `fb_count=1` + `CAMERA_GRAB_LATEST` (freeze fix), 80ms frame pacing (WiFi buffer fix), flash LED init after camera init (GPIO4 conflict fix).

---

### `web_ui_v7.h` — Dashboard

Full single-page app inlined as PROGMEM. Vanilla HTML/CSS/JS, no external runtime frameworks.

- Dark-glass UI, responsive 12-column grid
- Live temp/humidity, PIR 5s countdown
- Per-actuator open/close buttons (Win1, Win2, Curtain, Door)
- Reed confirmation bars with stuck-window alert + force-close button
- MJPEG camera stream + flash toggle + stream reload
- Master Activate/Deactivate button
- WebSocket auto-reconnect with 2.5s retry

---

## UART Communication Protocol

All messages are ASCII strings terminated with `\n`. 115200 baud, 8N1.

### ESP32 #1 → ESP32 #2 (Peripherals)

| Command | Action |
|---------|--------|
| `LIGHT_ON` / `LIGHT_OFF` | Energise / de-energise Relay 1 |
| `FAN_ON` / `FAN_OFF` | Energise / de-energise Relay 2 |
| `BEEP` | Single short buzzer beep |
| `BEEP_ALERT` | Three-beep alert pattern |
| `TEMP_REQ` | Request DHT22 reading |

### ESP32 #1 → ESP32 #3 (Motors)

| Command | Action |
|---------|--------|
| `WIN1_OPEN` / `WIN1_CLOSE` | Servo — Window 1 |
| `WIN2_OPEN` / `WIN2_CLOSE` | DC Motor B — Window 2 |
| `WIN1_FORCE_CLOSE` / `WIN2_FORCE_CLOSE` | Force close stuck window |
| `CURTAIN_OPEN` / `CURTAIN_CLOSE` | DC Motor A — Curtain |
| `DOOR_OPEN` / `DOOR_CLOSE` | Servo — Door |
| `ALL_STOP` | Emergency stop all motors + home servos |

> ⚠️ Commands auto-inverted by ESP32 #1 for `INV_WIN1=true` and `INV_DOOR=true` channels.

### ESP32 #3 → ESP32 #1 (Responses)

| Response | Meaning |
|----------|---------|
| `OK` | Command acknowledged |
| `DONE:WIN2` / `DONE:CURTAIN` | DC motor auto-stopped (3s timeout) |
| `ALERT:WIN1_STUCK` / `ALERT:WIN2_STUCK` | Reed not triggered — window obstructed |
| `WIN1_OPENED` / `WIN2_OPENED` | Reed triggered — open confirmed |
| `READY` | ESP32 #3 boot complete |
| `ERR:UNKNOWN` | Unrecognised command |

---

## Web Dashboard

Navigate to the IP shown on the TFT after boot (e.g. `http://192.168.x.x`).

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Full dashboard HTML |
| `/api/state` | GET | JSON snapshot of all states |
| `/api/cam` | GET | Camera stream and flash URLs |
| `/ws` | WebSocket | Bidirectional live state sync |

State is pushed every 2 seconds (heartbeat) and immediately on every event change.

---

## Setup & Flashing

### Prerequisites
- Arduino IDE 2.x or PlatformIO
- ESP32 board package: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- TFT_eSPI `User_Setup.h` configured for ILI9488, 480×320, VSPI pins

### Flash Order

1. **Configure WiFi** — set `WIFI_SSID` and `WIFI_PASSWORD` in `ESP32_Main.ino` and `CAM_v9.ino`
2. **Flash ESP32-CAM** — note Stream + Flash IP from Serial Monitor
3. **Update CAM IP** in `ESP32_Main.ino`:
   ```cpp
   const char* CAM_STREAM_URL = "http://<cam_ip>:81/stream";
   const char* CAM_FLASH_URL  = "http://<cam_ip>/flash";
   ```
4. **Flash ESP32 #3** (Motor Controller) — verify `READY` on Serial Monitor
5. **Flash ESP32 #2** (Peripherals Board) — verify relays don't click on boot
6. **Flash ESP32 #1** (Main Controller) — TFT shows IP after WiFi connects
7. **Verify motor directions** — toggle `INV_*` flags if any motor moves wrong way
8. **Verify reed switches** — confirm 10kΩ pull-up resistors on GPIO 36 & 39

---

## Libraries Required

### ESP32 #1 — Main Controller
| Library | Source |
|---------|--------|
| ESPAsyncWebServer | https://github.com/me-no-dev/ESPAsyncWebServer |
| AsyncTCP | https://github.com/me-no-dev/AsyncTCP |
| ArduinoJson | Arduino Library Manager |
| TFT_eSPI | Arduino Library Manager |

### ESP32 #2 — Peripherals Board
| Library | Source |
|---------|--------|
| DHT sensor library | Adafruit — Arduino Library Manager |
| Adafruit Unified Sensor | Adafruit — Arduino Library Manager |
| DFRobotDFPlayerMini | Arduino Library Manager |

### ESP32 #3 — Motor Controller
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
| Door stays open after reboot | `state.doorOpen` initialised as `true` | `state.doorOpen = false`; `DOOR_CLOSE` sent in `setup()` |
| PIR door never auto-closes | `pirDoorOpenedAt` reset every loop while PIR was HIGH | Timer reset only on rising edge |
| Master OFF does not close door | Door excluded from `applyMasterSwitch(false)` | `DOOR_CLOSE` added to both ON/OFF paths; PIR timer cancelled first |
| ESP32-CAM stream freezes | `fb_count=2` caused frame buffer desync | `fb_count=1` + `CAMERA_GRAB_LATEST` + 80ms frame pacing |
| Flash LED non-functional | GPIO4 LEDC conflict with camera init | `ledcAttach()` moved after `esp_camera_init()` |
| Motor commands physically inverted | WIN1 and DOOR wiring reversed | `INV_WIN1=true`, `INV_DOOR=true` swap UART string in `sendMotorCmd()` |
| ESP32-CAM error 112 (port conflict) | Two `httpd_start()` calls on same `ctrl_port` | Stream: port 81, `ctrl_port=32769`; Control: port 80 |
| Sequential UART commands dropped | ESP32 #3 still processing when next command arrived | 300ms `MOTOR_CMD_GAP_MS` between sequential UART sends |

---

## Team

| Name | Role |
|------|------|
| Gautam | Firmware development, hardware integration, system architecture |
| Shivangi Vishwakarma | Co-developer, project co-lead |

**Institution:** Thakur College of Engineering and Technology (TCET), Mumbai
**Program:** TCET Inhouse Internship
**Collaboration:** Agastya International Foundation
**Deployment Site:** Sanmitra Mandal Vidhyalaya, Goregaon East, Mumbai

---

## License

This project is developed for academic and educational purposes under the TCET Inhouse Internship program. Please contact the authors before reuse or redistribution.
