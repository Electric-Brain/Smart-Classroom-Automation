/*
 * Smart Classroom – ESP32 #2 (Motor Controller)
 * DOOR:
 *  - Starts at 12 o’clock (0°)
 *  - Opens to 6 o’clock (180°)
 *  - Moves ANTICLOCKWISE
 */

#include <ESP32Servo.h>

// ─── Pins ────────────────────────────────────────────────────
#define SERVO_DOOR_PIN     5
#define SERVO_WIN1_PIN    18
#define STBY_PIN          23
#define C_PWMA            19
#define C_AIN1            21
#define C_AIN2            22
#define W2_PWMB           25
#define W2_BIN1           26
#define W2_BIN2           27
#define UART2_RX          16
#define UART2_TX          17
#define UART_BAUD         115200

// ─── Servo angles ─────────────────────────────────────────────
#define DOOR_CLOSE_DEG     0     // 12 o'clock
#define DOOR_OPEN_DEG    180     // 6 o'clock
#define WIN1_OPEN_DEG      0
#define WIN1_CLOSE_DEG   180

// ─── DC motor config ──────────────────────────────────────────
#define PWM_FREQ        18000
#define PWM_RES            10
#define SPEED_NORMAL       400
#define MOTOR_TIMEOUT     3000UL

#define M_CURTAIN  0
#define M_WIN2     1

enum MotorState { IDLE, RUNNING };

struct DCMotor {
  const char* name;
  MotorState mstate = IDLE;
  unsigned long startMs = 0;
  int in1, in2, pwmPin, speed;
};

DCMotor dcMotors[2] = {
  {"CURTAIN", IDLE, 0, C_AIN1,  C_AIN2,  C_PWMA, SPEED_NORMAL},
  {"WIN2",    IDLE, 0, W2_BIN1, W2_BIN2, W2_PWMB, SPEED_NORMAL},
};

Servo servoDoor;
Servo servoWin1;

bool doorOpen = false;
bool win1Open = false;
String rxBuf = "";

// ─────────────────────────────────────────
// DOOR MOVEMENT (ANTICLOCKWISE)
// ─────────────────────────────────────────
void moveDoor(bool open) {
  servoDoor.write(open ? DOOR_OPEN_DEG : DOOR_CLOSE_DEG);
  doorOpen = open;
  Serial2.println("OK");
}

void moveWin1(bool open) {
  servoWin1.write(open ? WIN1_OPEN_DEG : WIN1_CLOSE_DEG);
  win1Open = open;
  Serial2.println("OK");
}

void motorStop(int id) {
  DCMotor& m = dcMotors[id];
  if (m.mstate == IDLE) return;
  digitalWrite(m.in1, LOW);
  digitalWrite(m.in2, LOW);
  ledcWrite(m.pwmPin, 0);
  m.mstate = IDLE;
}

void motorRun(int id, bool forward) {
  DCMotor& m = dcMotors[id];
  if (m.mstate == RUNNING) {
    motorStop(id);
    delay(80);
  }

  m.mstate = RUNNING;
  m.startMs = millis();

  digitalWrite(STBY_PIN, HIGH);
  digitalWrite(m.in1, forward ? HIGH : LOW);
  digitalWrite(m.in2, forward ? LOW  : HIGH);
  ledcWrite(m.pwmPin, m.speed);

  Serial2.println("OK");
}

void allStop() {
  for (int i = 0; i < 2; i++) motorStop(i);
  digitalWrite(STBY_PIN, LOW);

  servoDoor.write(DOOR_CLOSE_DEG);
  servoWin1.write(WIN1_CLOSE_DEG);

  doorOpen = false;
  win1Open = false;

  Serial2.println("OK");
}

void handleCommand(const String& cmd) {
  if      (cmd == "WIN1_OPEN")     moveWin1(true);
  else if (cmd == "WIN1_CLOSE")    moveWin1(false);
  else if (cmd == "WIN2_OPEN")     motorRun(M_WIN2, true);
  else if (cmd == "WIN2_CLOSE")    motorRun(M_WIN2, false);
  else if (cmd == "CURTAIN_OPEN")  motorRun(M_CURTAIN, true);
  else if (cmd == "CURTAIN_CLOSE") motorRun(M_CURTAIN, false);
  else if (cmd == "DOOR_OPEN")     moveDoor(true);
  else if (cmd == "DOOR_CLOSE")    moveDoor(false);
  else if (cmd == "ALL_STOP")      allStop();
  else                             Serial2.println("ERR:UNKNOWN");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART2_RX, UART2_TX);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  servoDoor.setPeriodHertz(50);
  servoWin1.setPeriodHertz(50);

  // 🔥 CRITICAL LINE – REVERSED PULSE FOR ANTICLOCKWISE
  servoDoor.attach(SERVO_DOOR_PIN, 2400, 500);

  // Normal direction for WIN1
  servoWin1.attach(SERVO_WIN1_PIN, 500, 2400);

  // Initial position = 12 o'clock
  servoDoor.write(DOOR_CLOSE_DEG);
  servoWin1.write(WIN1_CLOSE_DEG);

  pinMode(STBY_PIN, OUTPUT);
  digitalWrite(STBY_PIN, LOW);

  int dirs[] = {C_AIN1, C_AIN2, W2_BIN1, W2_BIN2};
  for (int p : dirs) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }

  ledcAttach(C_PWMA, PWM_FREQ, PWM_RES);
  ledcWrite(C_PWMA, 0);

  ledcAttach(W2_PWMB, PWM_FREQ, PWM_RES);
  ledcWrite(W2_PWMB, 0);

  Serial2.println("READY");
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 2; i++) {
    if (dcMotors[i].mstate == RUNNING &&
        (now - dcMotors[i].startMs) >= MOTOR_TIMEOUT) {
      motorStop(i);
      Serial2.println("DONE:" + String(dcMotors[i].name));
    }
  }

  while (Serial2.available()) {
    char c = Serial2.read();

    if (c == '\n') {
      rxBuf.trim();
      if (rxBuf.length()) handleCommand(rxBuf);
      rxBuf = "";
    }
    else if (c != '\r') {
      rxBuf += c;
    }
  }
}