/*
 * ============================================================
 *  Smart Classroom – ESP32-CAM  (AI Thinker)
 *  Fixes applied:
 *    1. Stream freeze  – fb_count=1 + CAMERA_GRAB_LATEST + frame pacing
 *    2. Flash not working – init flash AFTER camera, use correct GPIO4
 *    3. Sensor tuned for indoor lighting
 * ============================================================
 */

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include "esp_timer.h"

const char* WIFI_SSID     = "22";
const char* WIFI_PASSWORD = "12345678";

// ─── AI Thinker pin map ──────────────────────────────────────
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN      4
volatile int flashIntensity = 0;

// ─── MJPEG ───────────────────────────────────────────────────
#define PART_BOUNDARY "gc0lo"
static const char* STREAM_CONTENT_TYPE =
  "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART =
  "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Pace frames to ~12fps – prevents WiFi buffer overflow (main cause of freeze)
#define STREAM_FRAME_MS  80

httpd_handle_t stream_httpd = NULL;
httpd_handle_t ctrl_httpd   = NULL;

// ─── Stream handler ──────────────────────────────────────────
static esp_err_t stream_handler(httpd_req_t* req) {
  camera_fb_t* fb  = NULL;
  esp_err_t    res = ESP_OK;
  char         part_buf[64];
  int64_t      last_frame = 0;

  res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  while (true) {
    // Frame pacing – key fix for freeze
    int64_t now_ms  = esp_timer_get_time() / 1000LL;
    int64_t elapsed = now_ms - last_frame;
    if (elapsed < STREAM_FRAME_MS)
      vTaskDelay(pdMS_TO_TICKS(STREAM_FRAME_MS - elapsed));
    last_frame = esp_timer_get_time() / 1000LL;

    fb = esp_camera_fb_get();
    if (!fb) {
      vTaskDelay(pdMS_TO_TICKS(30));
      continue;  // skip bad frame, keep stream alive
    }

    size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, fb->len);
    res  = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);

    if (res != ESP_OK) break;  // client disconnected cleanly
  }
  return res;
}

// ─── Capture handler ─────────────────────────────────────────
static esp_err_t capture_handler(httpd_req_t* req) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { httpd_resp_send_500(req); return ESP_FAIL; }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

// ─── Flash handler ───────────────────────────────────────────
static esp_err_t flash_handler(httpd_req_t* req) {
  char query[32] = {0};
  char param[16] = {0};
  int  val = flashIntensity;

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    if (httpd_query_key_value(query, "intensity", param, sizeof(param)) == ESP_OK) {
      val = atoi(param);
      val = val < 0 ? 0 : (val > 255 ? 255 : val);
    }

  flashIntensity = val;
  ledcWrite(FLASH_LED_PIN, val);
  Serial.printf("[FLASH] %d\n", val);

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_type(req, "application/json");
  char resp[48];
  snprintf(resp, sizeof(resp), "{\"flash\":%d,\"ok\":true}", val);
  httpd_resp_sendstr(req, resp);
  return ESP_OK;
}

void startStreamServer() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port    = 81;
  cfg.ctrl_port      = 32769;
  cfg.stack_size     = 8192;
  httpd_uri_t u = { "/stream", HTTP_GET, stream_handler, NULL };
  if (httpd_start(&stream_httpd, &cfg) == ESP_OK)
    httpd_register_uri_handler(stream_httpd, &u);
  Serial.println("[CAM] Stream :81/stream");
}

void startCtrlServer() {
  httpd_config_t cfg   = HTTPD_DEFAULT_CONFIG();
  cfg.server_port      = 80;
  cfg.max_uri_handlers = 4;
  httpd_uri_t uF = { "/flash",   HTTP_GET, flash_handler,   NULL };
  httpd_uri_t uC = { "/capture", HTTP_GET, capture_handler, NULL };
  if (httpd_start(&ctrl_httpd, &cfg) == ESP_OK) {
    httpd_register_uri_handler(ctrl_httpd, &uF);
    httpd_register_uri_handler(ctrl_httpd, &uC);
  }
  Serial.println("[CAM] Control :80");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n[CAM] Booting");

  // ── 1. Camera init FIRST (uses LEDC ch0 for XCLK) ──────────
  camera_config_t camCfg;
  camCfg.ledc_channel = LEDC_CHANNEL_0;
  camCfg.ledc_timer   = LEDC_TIMER_0;
  camCfg.pin_d0 = Y2_GPIO_NUM; camCfg.pin_d1 = Y3_GPIO_NUM;
  camCfg.pin_d2 = Y4_GPIO_NUM; camCfg.pin_d3 = Y5_GPIO_NUM;
  camCfg.pin_d4 = Y6_GPIO_NUM; camCfg.pin_d5 = Y7_GPIO_NUM;
  camCfg.pin_d6 = Y8_GPIO_NUM; camCfg.pin_d7 = Y9_GPIO_NUM;
  camCfg.pin_xclk     = XCLK_GPIO_NUM;
  camCfg.pin_pclk     = PCLK_GPIO_NUM;
  camCfg.pin_vsync    = VSYNC_GPIO_NUM;
  camCfg.pin_href     = HREF_GPIO_NUM;
  camCfg.pin_sscb_sda = SIOD_GPIO_NUM;
  camCfg.pin_sscb_scl = SIOC_GPIO_NUM;
  camCfg.pin_pwdn     = PWDN_GPIO_NUM;
  camCfg.pin_reset    = RESET_GPIO_NUM;
  camCfg.xclk_freq_hz = 20000000;
  camCfg.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    camCfg.frame_size  = FRAMESIZE_VGA;   // 640x480
    camCfg.jpeg_quality= 15;
    camCfg.fb_count    = 1;               // KEY FIX: 1 not 2 – prevents desync freeze
    camCfg.fb_location = CAMERA_FB_IN_PSRAM;
    camCfg.grab_mode   = CAMERA_GRAB_LATEST; // always newest frame
  } else {
    camCfg.frame_size  = FRAMESIZE_QVGA;
    camCfg.jpeg_quality= 20;
    camCfg.fb_count    = 1;
    camCfg.fb_location = CAMERA_FB_IN_DRAM;
    camCfg.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&camCfg);
  if (err != ESP_OK) {
    Serial.printf("[CAM] INIT FAILED 0x%x – halting\n", err);
    while (true) delay(1000);
  }
  Serial.println("[CAM] Camera OK");

  // Indoor sensor tuning
  sensor_t* s = esp_camera_sensor_get();
  s->set_brightness(s, 1);
  s->set_contrast(s,   1);
  s->set_whitebal(s,   1);
  s->set_awb_gain(s,   1);
  s->set_wb_mode(s,    2);  // indoor
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 1);
  s->set_gain_ctrl(s, 1);
  // s->set_vflip(s, 1);    // uncomment if image upside-down
  // s->set_hmirror(s, 1);  // uncomment if image mirrored

  // ── 2. Flash LED – init AFTER camera so GPIO4 is free ───────
  ledcAttach(FLASH_LED_PIN, 5000, 8);
  ledcWrite(FLASH_LED_PIN, 0);
  Serial.println("[CAM] Flash LED ready (GPIO4)");

  // ── 3. WiFi ──────────────────────────────────────────────────
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[CAM] WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 > 20000) { Serial.println(" timeout – restart"); ESP.restart(); }
    delay(500); Serial.print('.');
  }
  Serial.println(" OK");

  IPAddress ip = WiFi.localIP();
  Serial.println("=========================================");
  Serial.printf("  Stream  : http://%s:81/stream\n", ip.toString().c_str());
  Serial.printf("  Flash   : http://%s/flash\n",     ip.toString().c_str());
  Serial.printf("  Capture : http://%s/capture\n",   ip.toString().c_str());
  Serial.println("  >>> Paste Stream + Flash URLs into ESP32_Main.ino <<<");
  Serial.println("=========================================");

  startStreamServer();
  startCtrlServer();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CAM] WiFi lost – reconnect");
    WiFi.reconnect();
    delay(5000);
  }
  delay(1000);
}
