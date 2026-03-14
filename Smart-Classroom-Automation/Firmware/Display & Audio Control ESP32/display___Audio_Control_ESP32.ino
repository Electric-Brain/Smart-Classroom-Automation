#include <TFT_eSPI.h>
#include <SPI.h>
#include <DFRobotDFPlayerMini.h>

// ========================== BOOKBUDDY V4 CONFIG ==========================
#define PIN_DFPLAYER_RX 16
#define PIN_DFPLAYER_TX 17
#define SLIDE_DURATION_MS 5000

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini df;
TFT_eSPI tft = TFT_eSPI();

// State Tracking
int currentLetter = 0;
unsigned long lastSlideTime = 0;
bool morningShown = false;
bool dfPlayerReady = false;
bool audioStarted = false;

#define BOARD_GREEN tft.color565(20, 60, 20)
#define WOOD_BROWN  tft.color565(100, 50, 20)
#define CHALK_WHITE tft.color565(240, 240, 240)

// *** CHANGE THIS to the track number of your audio file on the SD card ***
#define AUDIO_TRACK 1

const char* words[] = {
  "APPLE", "BAT", "CUP", "DRUM", "EGG", "FAN", "GLASS", "HAT", "IGLOO", "JAR",
  "KITE", "LEAF", "MOON", "NET", "ORANGE", "PEN", "QUILT", "RING", "SUN", "TREE",
  "UMBRELLA", "VAN", "WATCH", "X-BOX", "YOYO", "ZEBRA"
};

// ========================== CORE FUNCTIONS ==========================
void chalkWipeTransition() {
  for (int y = 0; y < tft.height(); y += 15) {
    tft.drawFastHLine(0, y, tft.width(), CHALK_WHITE);
    if (y > 15) tft.fillRect(0, 0, tft.width(), y - 15, BOARD_GREEN);
    delay(5);
  }
  tft.fillRect(0, 0, tft.width(), tft.height(), BOARD_GREEN);
  tft.drawRect(0, 0, tft.width(), tft.height(), WOOD_BROWN);
}

void showGoodMorning() {
  for (int i = 0; i < tft.height(); i++) {
    tft.drawFastHLine(0, i, tft.width(), tft.color565(135 - (i / 6), 206 - (i / 6), 235));
  }
  tft.fillEllipse(120, 320, 200, 100, tft.color565(34, 139, 34));
  tft.fillCircle(400, 60, 40, TFT_YELLOW);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(6);
  tft.drawString("GOOD", 240, 80);
  tft.setTextColor(tft.color565(255, 140, 0));
  tft.drawString("MORNING", 240, 160);
  delay(3000);
}

void showSlide(int index) {
  // *** Start audio on very first slide (A), then never touch it again ***
  if (!audioStarted && dfPlayerReady) {
    df.playMp3Folder(AUDIO_TRACK);
    audioStarted = true;
    Serial.println("Audio started.");
  }

  chalkWipeTransition();

  tft.setTextDatum(MC_DATUM);
  char letter[2] = {(char)('A' + index), '\0'};
  tft.setTextSize(26);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(letter, tft.width() / 4, tft.height() / 2);
  tft.setTextSize(4);
  tft.setTextColor(TFT_BLACK);
  String label = String(letter) + " FOR " + words[index];
  tft.drawString(label, tft.width() / 2, tft.height() - 45);
}

// ========================== AUDIO LOOP HANDLER ==========================
void handleAudioLoop() {
  if (!dfPlayerReady || !audioStarted) return;

  if (df.available()) {
    uint8_t type = df.readType();
    int value = df.read();

    if (type == DFPlayerPlayFinished) {
      Serial.println("Audio finished — restarting loop...");
      delay(300); // Brief pause before restart
      df.playMp3Folder(AUDIO_TRACK); // Restart the same track
    }

    if (type == DFPlayerError) {
      Serial.print("DFPlayer Error: ");
      Serial.println(value);
      // Try to recover by replaying
      delay(500);
      df.playMp3Folder(AUDIO_TRACK);
    }
  }
}

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  dfSerial.begin(9600, SERIAL_8N1, PIN_DFPLAYER_RX, PIN_DFPLAYER_TX);

  if (!df.begin(dfSerial, false, true)) {
    Serial.println("DFPlayer Error - Check wiring");
    dfPlayerReady = false;
  } else {
    df.volume(23);
    dfPlayerReady = true;
    Serial.println("DFPlayer Ready");
  }
}

void loop() {
  if (!morningShown) {
    showGoodMorning();
    morningShown = true;
    return;
  }

  // Always keep checking if audio needs to restart
  handleAudioLoop();

  // Advance slides independently on their own timer
  if (millis() - lastSlideTime >= SLIDE_DURATION_MS) {
    showSlide(currentLetter);
    currentLetter++;
    if (currentLetter > 25) currentLetter = 0;
    lastSlideTime = millis();
  }
}