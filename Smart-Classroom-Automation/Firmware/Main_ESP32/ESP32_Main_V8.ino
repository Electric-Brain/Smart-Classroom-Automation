/*
 * ════════════════════════════════════════════════════════════════
 *  Smart Classroom  –  ESP32 #1  (Main Controller)  v6
 * ════════════════════════════════════════════════════════════════
 *  FIXES v6:
 *    • Door always initialises CLOSED; PIR opens it for 5 s then
 *      auto-closes (was: door stayed open after reboot).
 *    • Master switch (touch + web button) correctly opens ALL
 *      actuators on ON, closes ALL on OFF – including door.
 *    • SD / audio completely removed (hardware + API + state).
 *    • Web UI rebuilt: cleaner, professional dark-glass theme,
 *      no audio panel, no SD upload widget.
 *
 *  Hardware on this board:
 *    • ILI9488 TFT  480×320  (VSPI)
 *    • DHT22  sensor
 *    • 2× Relay      (light + fan, active-LOW opto-isolated)
 *    • Buzzer        active, GPIO 5
 *    • PIR sensor    GPIO 35
 *    • TTP223        touch sensor  GPIO 34
 *    • UART2         → ESP32 #2 motor controller
 *
 *  ── PIN MAP ────────────────────────────────────────────────────
 *  TFT ILI9488 (VSPI)
 *    CS   → GPIO 15
 *    DC   → GPIO  2
 *    RST  → GPIO  4
 *    MOSI → GPIO 23
 *    MISO → GPIO 19
 *    SCK  → GPIO 18
 *
 *  DHT22  DATA → GPIO 26
 *  Relay  LIGHT → GPIO 32
 *  Relay  FAN   → GPIO 33
 *  Buzzer       → GPIO  5
 *  PIR          → GPIO 35
 *  Touch TTP223 → GPIO 34
 *  UART2  TX    → GPIO 17   → ESP32#2 RX
 *  UART2  RX    → GPIO 16   ← ESP32#2 TX
 *
 *  Reed (input-only, external 10k pull-up to 3.3V required):
 *    REED1 → GPIO 36   (Window 1)
 *    REED2 → GPIO 39   (Window 2)
 * ════════════════════════════════════════════════════════════════
 */

#include <WiFi.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <TFT_eSPI.h>
#include "web_ui_v7.h"   // ← new clean UI header

// ─── WiFi credentials ────────────────────────────────────────
const char* WIFI_SSID     = "22";
const char* WIFI_PASSWORD = "12345678";

// ─── ESP32-CAM URLs ──────────────────────────────────────────
const char* CAM_STREAM_URL = "http://10.152.138.112:81/stream";
const char* CAM_FLASH_URL  = "http://10.152.138.112/flash";

// ─── Pin definitions ─────────────────────────────────────────
#define DHT_PIN        26
#define DHT_TYPE       DHT22
#define RELAY_LIGHT    32
#define RELAY_FAN      33
#define BUZZER_PIN      5
#define TOUCH_PIN      34
#define PIR_PIN        35
#define UART2_TX       17
#define UART2_RX       16
#define UART_BAUD      115200
#define REED1_PIN      36
#define REED2_PIN      39

// ─── Objects ─────────────────────────────────────────────────
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DHT            dht(DHT_PIN, DHT_TYPE);
TFT_eSPI       tft;

// ─── State ───────────────────────────────────────────────────
struct State {
  bool   lightOn     = false;
  bool   fanOn       = false;
  bool   win1Open    = false;
  bool   win2Open    = false;
  bool   curtainOpen = false;
  bool   doorOpen    = false;   // FIX: starts false (closed)
  float  tempC       = 0;
  float  humidity    = 0;
  String motorStatus = "IDLE";
  bool   classroomOn = false;
  bool   pirMotion   = false;
  bool   win1Alert   = false;
  bool   win2Alert   = false;
  bool   reed1Open   = false;
  bool   reed2Open   = false;
} state;

// ─── PIR auto-door ───────────────────────────────────────────
#define PIR_DOOR_OPEN_MS  5000UL
bool          pirDoorActive   = false;
unsigned long pirDoorOpenedAt = 0;

// ─── Touch sensor ────────────────────────────────────────────
bool          lastTouchRaw    = false;
bool          touchActive     = false;
unsigned long touchDebounceMs = 0;
unsigned long lastTouchAction = 0;
#define TOUCH_DEBOUNCE    50UL
#define TOUCH_REFIRE_MS  800UL

// ─── Timing ──────────────────────────────────────────────────
unsigned long lastDHT = 0, lastTFT = 0, lastWsHB = 0;
#define DHT_INTERVAL    3000
#define TFT_INTERVAL    1000
#define WS_HB_INTERVAL  2000

// ─── UART RX buffer ──────────────────────────────────────────
String uartRxBuf = "";

// ─── Buzzer helpers ──────────────────────────────────────────
void beep(int ms = 80) {
  digitalWrite(BUZZER_PIN, HIGH); delay(ms); digitalWrite(BUZZER_PIN, LOW);
}
void beepPattern(int n) {
  for (int i = 0; i < n; i++) { beep(60); if (i < n-1) delay(80); }
}
void beepAlert() {
  beep(300); delay(80); beep(80); delay(80); beep(300);
}

// ─── UART to motor controller ────────────────────────────────
// INVERSION FLAGS – set true if that motor's open/close are physically
// reversed (ESP32 #2 wiring or code inverts direction for that channel).
// Flip these to fix wrong direction without touching ESP32 #2.
#define INV_WIN1  true    // Window 1 open/close are swapped
#define INV_WIN2  false   // Window 2 is correct
#define INV_CURTAIN false // Curtain is correct
#define INV_DOOR  true    // Door open/close are swapped

// Sends a motor command, automatically inverting OPEN↔CLOSE for channels
// that are flagged above. State variables are always set correctly by the
// caller – only the UART string is flipped here.
void sendMotorCmd(const String& cmd) {
  String out = cmd;

  auto inv = [&](const char* base, bool doInvert) {
    String open_s  = String(base) + "_OPEN";
    String close_s = String(base) + "_CLOSE";
    if (doInvert) {
      if (cmd == open_s)  out = close_s;
      if (cmd == close_s) out = open_s;
    }
  };

  inv("WIN1",    INV_WIN1);
  inv("WIN2",    INV_WIN2);
  inv("CURTAIN", INV_CURTAIN);
  inv("DOOR",    INV_DOOR);

  Serial2.println(out);
  Serial.printf("[MOTOR] %s → UART: %s\n", cmd.c_str(), out.c_str());
}

// ─── Broadcast state ─────────────────────────────────────────
void broadcastState() {
  StaticJsonDocument<400> doc;
  doc["light"]     = state.lightOn;
  doc["fan"]       = state.fanOn;
  doc["win1"]      = state.win1Open;
  doc["win2"]      = state.win2Open;
  doc["curtain"]   = state.curtainOpen;
  doc["door"]      = state.doorOpen;
  doc["temp"]      = state.tempC;
  doc["humidity"]  = state.humidity;
  doc["motor"]     = state.motorStatus;
  doc["classOn"]   = state.classroomOn;
  doc["pir"]       = state.pirMotion;
  doc["win1Alert"] = state.win1Alert;
  doc["win2Alert"] = state.win2Alert;
  doc["reed1Open"] = state.reed1Open;
  doc["reed2Open"] = state.reed2Open;
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

// ─── Master switch ────────────────────────────────────────────
// FIX: Always sends all motor commands unconditionally.
// ON  → light, fan, win1, win2, curtain, door all OPEN.
// OFF → light, fan off; win1, win2, curtain, door all CLOSED.
// Door is included in both directions.
// ── Helper: send a motor command and wait for ESP32 #2 to fully
//    process it before sending the next one.
//    300 ms is safe for any motor controller that echoes a response
//    or just needs time to parse + act on a command.
#define MOTOR_CMD_GAP_MS  300

void applyMasterSwitch(bool turnOn) {
  state.classroomOn = turnOn;

  if (!turnOn) {
    // ── Turn everything OFF ──────────────────────────────────
    state.lightOn = false; digitalWrite(RELAY_LIGHT, HIGH);
    state.fanOn   = false; digitalWrite(RELAY_FAN,   HIGH);

    // Cancel any active PIR timer FIRST so it can't re-open door
    pirDoorActive = false;

    // Send CLOSE commands with generous gaps so ESP32 #2 processes each one
    sendMotorCmd("DOOR_CLOSE");    state.doorOpen    = false;
    delay(MOTOR_CMD_GAP_MS);
    sendMotorCmd("WIN1_CLOSE");    state.win1Open    = false; state.win1Alert = false;
    delay(MOTOR_CMD_GAP_MS);
    sendMotorCmd("WIN2_CLOSE");    state.win2Open    = false; state.win2Alert = false;
    delay(MOTOR_CMD_GAP_MS);
    sendMotorCmd("CURTAIN_CLOSE"); state.curtainOpen = false;

    state.motorStatus = "CLOSING ALL";
    beepPattern(2);
    Serial.println("[MASTER] Classroom OFF");

  } else {
    // ── Turn everything ON ───────────────────────────────────
    state.lightOn = true; digitalWrite(RELAY_LIGHT, LOW);
    state.fanOn   = true; digitalWrite(RELAY_FAN,   LOW);

    // Send OPEN commands with generous gaps so ESP32 #2 processes each one
    sendMotorCmd("WIN1_OPEN");    state.win1Open    = true;
    delay(MOTOR_CMD_GAP_MS);
    sendMotorCmd("WIN2_OPEN");    state.win2Open    = true;
    delay(MOTOR_CMD_GAP_MS);
    sendMotorCmd("CURTAIN_OPEN"); state.curtainOpen = true;
    delay(MOTOR_CMD_GAP_MS);
    sendMotorCmd("DOOR_OPEN");    state.doorOpen    = true;

    state.motorStatus = "OPENING ALL";
    beepPattern(3);
    Serial.println("[MASTER] Classroom ON");
  }

  broadcastState();
}

// ─── Touch sensor ─────────────────────────────────────────────
void pollTouch() {
  bool raw = (digitalRead(TOUCH_PIN) == HIGH);
  unsigned long now = millis();
  if (raw != lastTouchRaw) { touchDebounceMs = now; lastTouchRaw = raw; }
  if ((now - touchDebounceMs) >= TOUCH_DEBOUNCE) {
    if (raw && !touchActive) {
      touchActive = true;
      if (now - lastTouchAction >= TOUCH_REFIRE_MS) {
        lastTouchAction = now;
        applyMasterSwitch(!state.classroomOn); // FIX: toggles full master state
      }
    } else if (!raw) {
      touchActive = false;
    }
  }
}

// ─── PIR sensor (with auto door open/close) ──────────────────
// Door starts CLOSED. PIR rising edge opens it and starts a 5 s
// countdown. Timer only resets on a NEW rising edge, so the door
// will always close 5 s after the last new motion trigger.
// BUG FIXED: removed per-loop timer reset that prevented closing.
void pollPIR() {
  bool raw = (digitalRead(PIR_PIN) == HIGH);
  unsigned long now = millis();

  // ── Rising edge: new motion detected ─────────────────────
  if (raw && !state.pirMotion) {
    state.pirMotion = true;
    Serial.println("[PIR] Motion detected → opening door");

    if (!state.doorOpen) {
      sendMotorCmd("DOOR_OPEN");
      state.doorOpen    = true;
      state.motorStatus = "DOOR AUTO-OPEN (PIR)";
      beep(80);
    }
    // Reset 5-second timer on every new rising edge
    pirDoorActive   = true;
    pirDoorOpenedAt = now;
    broadcastState();

  // ── Falling edge: motion cleared ─────────────────────────
  } else if (!raw && state.pirMotion) {
    state.pirMotion = false;
    Serial.println("[PIR] Motion cleared");
    broadcastState();
  }

  // ── Auto-close after 5 s with no new trigger ─────────────
  // This now reliably fires because we do NOT reset pirDoorOpenedAt
  // every loop iteration while PIR is HIGH.
  if (pirDoorActive && (now - pirDoorOpenedAt >= PIR_DOOR_OPEN_MS)) {
    pirDoorActive = false;
    if (state.doorOpen) {
      Serial.println("[PIR] 5 s elapsed → closing door");
      sendMotorCmd("DOOR_CLOSE");
      state.doorOpen    = false;
      state.motorStatus = "DOOR AUTO-CLOSE (PIR)";
      broadcastState();
    }
  }
}

// ─── Reed switch poll (debounced) ────────────────────────────
#define REED_SAMPLE_MS    50UL
#define REED_STABLE_COUNT  5

void pollReed() {
  static unsigned long lastSampleMs = 0;
  static bool raw1Last = false, raw2Last = false;
  static uint8_t stable1Count = 0, stable2Count = 0;
  static bool confirmed1 = false, confirmed2 = false;

  unsigned long now = millis();
  if (now - lastSampleMs < REED_SAMPLE_MS) return;
  lastSampleMs = now;

  bool raw1 = (digitalRead(REED1_PIN) == HIGH);
  bool raw2 = (digitalRead(REED2_PIN) == HIGH);

  if (raw1 == raw1Last) { if (stable1Count < REED_STABLE_COUNT) stable1Count++; }
  else                  { stable1Count = 0; }
  raw1Last = raw1;

  if (raw2 == raw2Last) { if (stable2Count < REED_STABLE_COUNT) stable2Count++; }
  else                  { stable2Count = 0; }
  raw2Last = raw2;

  bool changed = false;
  if (stable1Count >= REED_STABLE_COUNT && raw1 != confirmed1) {
    confirmed1 = raw1; state.reed1Open = confirmed1;
    Serial.printf("[REED1] stable → %s\n", confirmed1 ? "OPEN" : "CLOSED");
    changed = true;
  }
  if (stable2Count >= REED_STABLE_COUNT && raw2 != confirmed2) {
    confirmed2 = raw2; state.reed2Open = confirmed2;
    Serial.printf("[REED2] stable → %s\n", confirmed2 ? "OPEN" : "CLOSED");
    changed = true;
  }
  if (changed) broadcastState();
}

// ─── TFT display ──────────────────────────────────────────────
void updateTFT() {
  static TFT_eSprite spr = TFT_eSprite(&tft);
  static bool init = false;
  if (!init) { spr.createSprite(480, 320); init = true; }

  spr.fillSprite(TFT_BLACK);

  // Header bar
  spr.fillRect(0, 0, 480, 34, 0x0C2A);
  spr.setTextColor(TFT_CYAN, 0x0C2A); spr.setTextSize(2);
  spr.setCursor(8, 8); spr.print("SMART CLASSROOM");

  uint16_t pillC = state.classroomOn ? TFT_GREEN : TFT_RED;
  spr.fillRoundRect(310, 6, 162, 22, 8, pillC);
  spr.setTextColor(TFT_BLACK, pillC); spr.setTextSize(1);
  spr.setCursor(318, 12);
  spr.print(state.classroomOn ? "CLASSROOM  ON " : "CLASSROOM  OFF");

  spr.fillCircle(466, 17, 8, touchActive ? TFT_YELLOW : 0x2104);
  spr.setTextColor(TFT_BLACK, touchActive ? TFT_YELLOW : 0x2104);
  spr.setCursor(461, 13); spr.print("T");

  spr.fillCircle(446, 17, 8, state.pirMotion ? TFT_ORANGE : 0x2104);
  spr.setTextColor(TFT_BLACK, state.pirMotion ? TFT_ORANGE : 0x2104);
  spr.setCursor(441, 13); spr.print("M");

  // Sensors
  spr.setTextSize(2); spr.setTextColor(TFT_ORANGE, TFT_BLACK);
  spr.setCursor(8, 42); spr.printf("%.1f", state.tempC);
  spr.setTextSize(1); spr.setTextColor(0x8410, TFT_BLACK);
  spr.setCursor(68, 45); spr.print("C");
  spr.setTextSize(2); spr.setTextColor(TFT_CYAN, TFT_BLACK);
  spr.setCursor(90, 42); spr.printf("%.0f%%", state.humidity);
  spr.setTextSize(1); spr.setTextColor(0x8410, TFT_BLACK);
  spr.setCursor(150, 45); spr.print("RH");

  spr.setTextColor(0x4208, TFT_BLACK);
  spr.setCursor(8, 62); spr.print("IP: "); spr.print(WiFi.localIP().toString());

  // Relays
  uint16_t lc = state.lightOn ? 0x07E0 : 0xC000;
  spr.fillRoundRect(8, 78, 90, 34, 6, lc);
  spr.setTextColor(TFT_WHITE, lc); spr.setTextSize(1);
  spr.setCursor(14, 83); spr.print("LIGHT");
  spr.setTextSize(2); spr.setCursor(14, 94); spr.print(state.lightOn ? "ON " : "OFF");

  uint16_t fc = state.fanOn ? 0x07E0 : 0xC000;
  spr.fillRoundRect(106, 78, 90, 34, 6, fc);
  spr.setTextColor(TFT_WHITE, fc); spr.setTextSize(1);
  spr.setCursor(112, 83); spr.print("FAN");
  spr.setTextSize(2); spr.setCursor(112, 94); spr.print(state.fanOn ? "ON " : "OFF");

  // Motor cards
  spr.setTextColor(TFT_WHITE, TFT_BLACK); spr.setTextSize(1);
  spr.setCursor(8, 122); spr.print("MOTORS");

  struct MCard { const char* name; bool open; };
  MCard mc[4] = {
    {"WIN 1",   state.win1Open},
    {"WIN 2",   state.win2Open},
    {"CURTAIN", state.curtainOpen},
    {"DOOR",    state.doorOpen},
  };
  for (int i = 0; i < 4; i++) {
    int x = 8 + i * 118;
    uint16_t bg = mc[i].open ? 0x0339 : 0x2945;
    spr.fillRoundRect(x, 132, 112, 42, 6, bg);
    spr.setTextColor(TFT_CYAN, bg); spr.setTextSize(1);
    spr.setCursor(x + 5, 136); spr.print(mc[i].name);
    spr.setTextColor(mc[i].open ? TFT_GREEN : 0xFB00, bg);
    spr.setTextSize(2); spr.setCursor(x + 5, 148);
    spr.print(mc[i].open ? "OPEN" : "CLSD");
  }

  spr.setTextSize(1); spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.setCursor(8, 184); spr.printf("CMD: %-28s", state.motorStatus.c_str());

  // Stuck alert
  if (state.win1Alert || state.win2Alert) {
    spr.fillRect(0, 176, 480, 14, 0xC800);
    spr.setTextColor(TFT_WHITE, 0xC800);
    spr.setCursor(8, 179);
    String alertTxt = "STUCK: ";
    if (state.win1Alert) alertTxt += "WIN1 ";
    if (state.win2Alert) alertTxt += "WIN2 ";
    alertTxt += " – check web UI";
    spr.print(alertTxt);
  }

  // Reed status
  spr.setTextSize(1);
  uint16_t r1c = state.reed1Open ? TFT_GREEN : TFT_RED;
  uint16_t r2c = state.reed2Open ? TFT_GREEN : TFT_RED;
  spr.setCursor(8, 196);
  spr.setTextColor(TFT_WHITE, TFT_BLACK); spr.print("REED ");
  spr.setTextColor(TFT_BLACK, r1c);
  spr.printf(" R1:%s ", state.reed1Open ? "OPN" : "CLS");
  spr.setTextColor(TFT_BLACK, r2c);
  spr.printf(" R2:%s ", state.reed2Open ? "OPN" : "CLS");

  spr.drawFastHLine(0, 208, 480, 0x2945);

  int rssi = WiFi.RSSI();
  spr.setTextColor(rssi > -60 ? TFT_GREEN : rssi > -75 ? TFT_YELLOW : TFT_RED, TFT_BLACK);
  spr.setCursor(8, 214); spr.printf("WiFi: %d dBm", rssi);

  spr.pushSprite(0, 0);
}

// ─── WebSocket event handler ─────────────────────────────────
void onWsEvent(AsyncWebSocket* s, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  // FIX: Send full state to every newly connected browser immediately.
  // Without this, if ESP32 #2 moves a motor on boot, the UI stays stale
  // until the next 2-second heartbeat.
  if (type == WS_EVT_CONNECT) {
    broadcastState();
    return;
  }
  if (type != WS_EVT_DATA) return;
  String msg = String((char*)data).substring(0, len);

  StaticJsonDocument<192> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) return;
  const char* cmd = doc["cmd"];
  if (!cmd) return;

  if      (strcmp(cmd,"LIGHT_ON")          ==0){ state.lightOn=true;  digitalWrite(RELAY_LIGHT,LOW);  beep(40); }
  else if (strcmp(cmd,"LIGHT_OFF")         ==0){ state.lightOn=false; digitalWrite(RELAY_LIGHT,HIGH); }
  else if (strcmp(cmd,"FAN_ON")            ==0){ state.fanOn=true;    digitalWrite(RELAY_FAN,LOW);    beep(40); }
  else if (strcmp(cmd,"FAN_OFF")           ==0){ state.fanOn=false;   digitalWrite(RELAY_FAN,HIGH);   }
  else if (strcmp(cmd,"WIN1_OPEN")         ==0){ sendMotorCmd("WIN1_OPEN");         state.win1Open=true;  state.win1Alert=false; state.motorStatus="WIN1 OPENING"; }
  else if (strcmp(cmd,"WIN1_CLOSE")        ==0){ sendMotorCmd("WIN1_CLOSE");        state.win1Open=false; state.win1Alert=false; state.motorStatus="WIN1 CLOSING"; }
  else if (strcmp(cmd,"WIN2_OPEN")         ==0){ sendMotorCmd("WIN2_OPEN");         state.win2Open=true;  state.win2Alert=false; state.motorStatus="WIN2 OPENING"; }
  else if (strcmp(cmd,"WIN2_CLOSE")        ==0){ sendMotorCmd("WIN2_CLOSE");        state.win2Open=false; state.win2Alert=false; state.motorStatus="WIN2 CLOSING"; }
  else if (strcmp(cmd,"WIN1_FORCE_CLOSE")  ==0){ sendMotorCmd("WIN1_FORCE_CLOSE"); state.win1Alert=false; state.motorStatus="WIN1 FORCE CLOSE"; beep(80); }
  else if (strcmp(cmd,"WIN2_FORCE_CLOSE")  ==0){ sendMotorCmd("WIN2_FORCE_CLOSE"); state.win2Alert=false; state.motorStatus="WIN2 FORCE CLOSE"; beep(80); }
  else if (strcmp(cmd,"WIN1_RETRY")        ==0){ sendMotorCmd("WIN1_FORCE_CLOSE"); state.win1Alert=false; state.motorStatus="WIN1 RETRY"; beep(60); }
  else if (strcmp(cmd,"WIN2_RETRY")        ==0){ sendMotorCmd("WIN2_FORCE_CLOSE"); state.win2Alert=false; state.motorStatus="WIN2 RETRY"; beep(60); }
  else if (strcmp(cmd,"CURTAIN_OPEN")      ==0){ sendMotorCmd("CURTAIN_OPEN");  state.curtainOpen=true;  state.motorStatus="CURTAIN OPENING"; }
  else if (strcmp(cmd,"CURTAIN_CLOSE")     ==0){ sendMotorCmd("CURTAIN_CLOSE"); state.curtainOpen=false; state.motorStatus="CURTAIN CLOSING"; }
  else if (strcmp(cmd,"DOOR_OPEN")         ==0){ sendMotorCmd("DOOR_OPEN");     state.doorOpen=true;     state.motorStatus="DOOR OPENING"; }
  else if (strcmp(cmd,"DOOR_CLOSE")        ==0){ sendMotorCmd("DOOR_CLOSE");    state.doorOpen=false;    state.motorStatus="DOOR CLOSING"; }
  else if (strcmp(cmd,"ALL_STOP")          ==0){ sendMotorCmd("ALL_STOP");      state.motorStatus="STOPPED"; }
  else if (strcmp(cmd,"MASTER_ON")         ==0) applyMasterSwitch(true);
  else if (strcmp(cmd,"MASTER_OFF")        ==0) applyMasterSwitch(false);

  broadcastState();
}

// ─── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART2_RX, UART2_TX);

  pinMode(RELAY_LIGHT, OUTPUT); digitalWrite(RELAY_LIGHT, HIGH);
  pinMode(RELAY_FAN,   OUTPUT); digitalWrite(RELAY_FAN,   HIGH);
  pinMode(BUZZER_PIN,  OUTPUT); digitalWrite(BUZZER_PIN,  LOW);
  pinMode(TOUCH_PIN,   INPUT);
  pinMode(PIR_PIN,     INPUT);
  // FIX: Ensure door is physically closed on boot
  sendMotorCmd("DOOR_CLOSE");
  state.doorOpen = false;
  // GPIO36 & 39: input-only, no internal pull-up – use external 10kΩ
  pinMode(REED1_PIN, INPUT);
  pinMode(REED2_PIN, INPUT);

  dht.begin();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN); tft.setTextSize(2);
  tft.setCursor(8, 8);  tft.println("Smart Classroom");
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
  tft.setCursor(8, 36); tft.println("Booting...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting");
  tft.setCursor(8, 52); tft.print("WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print('.'); }
  Serial.println("\n[WiFi] " + WiFi.localIP().toString());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r){
    r->send_P(200, "text/html", DASHBOARD_HTML);
  });

  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* r){
    StaticJsonDocument<400> doc;
    doc["light"]     = state.lightOn;    doc["fan"]       = state.fanOn;
    doc["win1"]      = state.win1Open;   doc["win2"]      = state.win2Open;
    doc["curtain"]   = state.curtainOpen; doc["door"]     = state.doorOpen;
    doc["temp"]      = state.tempC;      doc["humidity"]  = state.humidity;
    doc["motor"]     = state.motorStatus; doc["classOn"]  = state.classroomOn;
    doc["pir"]       = state.pirMotion;
    doc["win1Alert"] = state.win1Alert;  doc["win2Alert"] = state.win2Alert;
    doc["reed1Open"] = state.reed1Open;  doc["reed2Open"] = state.reed2Open;
    String o; serializeJson(doc, o);
    r->send(200, "application/json", o);
  });

  server.on("/api/cam", HTTP_GET, [](AsyncWebServerRequest* r){
    StaticJsonDocument<128> d;
    d["stream"] = CAM_STREAM_URL; d["flash"] = CAM_FLASH_URL;
    String o; serializeJson(d, o); r->send(200, "application/json", o);
  });

  server.begin();
  Serial.println("[HTTP] http://" + WiFi.localIP().toString());
  beep(150); delay(120); beep(150);
  updateTFT();
}

// ─── Main loop ───────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  pollTouch();
  pollPIR();
  pollReed();

  if (now - lastDHT >= DHT_INTERVAL) {
    lastDHT = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) { state.tempC = t; state.humidity = h; }
  }

  // UART RX from motor controller
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      uartRxBuf.trim();
      if (uartRxBuf.length()) {
        Serial.println("[UART RX] " + uartRxBuf);

        if (uartRxBuf == "DONE:WIN1") {
          state.reed1Open = false; state.win1Alert = false; state.win1Open = false;
          state.motorStatus = "WIN1 CLOSED OK";
          broadcastState();
        } else if (uartRxBuf == "DONE:WIN2") {
          state.reed2Open = false; state.win2Alert = false; state.win2Open = false;
          state.motorStatus = "WIN2 CLOSED OK";
          broadcastState();
        } else if (uartRxBuf == "ALERT:WIN1_STUCK") {
          state.reed1Open = true; state.win1Alert = true; state.win1Open = true;
          state.motorStatus = "WIN1 STUCK!";
          beepAlert(); broadcastState();
        } else if (uartRxBuf == "ALERT:WIN2_STUCK") {
          state.reed2Open = true; state.win2Alert = true; state.win2Open = true;
          state.motorStatus = "WIN2 STUCK!";
          beepAlert(); broadcastState();
        } else if (uartRxBuf == "WIN1_OPENED") {
          state.reed1Open = true; state.win1Alert = false; broadcastState();
        } else if (uartRxBuf == "WIN2_OPENED") {
          state.reed2Open = true; state.win2Alert = false; broadcastState();
        } else {
          state.motorStatus = uartRxBuf;
          broadcastState();
        }
      }
      uartRxBuf = "";
    } else uartRxBuf += c;
  }

  if (now - lastTFT >= TFT_INTERVAL)   { lastTFT  = now; updateTFT(); }
  if (now - lastWsHB >= WS_HB_INTERVAL){ lastWsHB  = now; broadcastState(); ws.cleanupClients(); }
  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
}
