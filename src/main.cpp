#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>
#include "OLEDScreen.h"

// ===================== Hoverboard List =====================
struct BoardEntry {
  const char* name;
  uint8_t mac[6];
};
static const BoardEntry BOARDS[] = {
  {"H1", {0xAC, 0x67, 0xB2, 0x53, 0x86, 0x28}},
  {"H2", {0xE8, 0xDB, 0x84, 0x03, 0xF1, 0x80}},
};
static const size_t NUM_BOARDS = sizeof(BOARDS) / sizeof(BOARDS[0]);

// ===================== Pin Definitions (ESP32-C3) =====================
#define BUTTON_PIN  GPIO_NUM_0
#define POT_X_PIN   GPIO_NUM_2
#define POT_Y_PIN   GPIO_NUM_1
#define LED_PIN     GPIO_NUM_8

// ===================== Packet Format =====================
struct __attribute__((packed)) ControlPacket {
  int16_t speed_pct;
  int16_t steer_pct;
  uint8_t flags;
  uint8_t seq;
  uint16_t crc;
};
static uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF) {
  while (len--) {
    crc ^= ((uint16_t)*data++) << 8;
    for (int i = 0; i < 8; i++) crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
  }
  return crc;
}

// ===================== Globals =====================
OLEDScreen screen;
Preferences prefs;

enum UiState { RUN, MENU, SELECT };
UiState ui = RUN;

int centerX = 2048, centerY = 2048;
int deadzone = 180;
bool optInvSpeed = false;
bool optInvSteer = false;
bool optLog = true;

uint8_t seq = 0;
uint32_t tLastSend = 0, tLastOLED = 0;
const unsigned SEND_MS = 20;
const unsigned OLED_MS = 100;

bool btnPrev = false;
uint32_t tBtnDown = 0;
const unsigned LONG_PRESS_MENU_MS   = 5000;
const unsigned LONG_PRESS_SELECT_MS = 8000;

bool menuShown = false;
bool selectShown = false;

uint8_t PEER_MAC[6] = {0};
int selectIdx = 0;

static bool peerAdded = false;
void onEspNowSent(const uint8_t*, esp_now_send_status_t) {}

// ===================== Helpers =====================
static inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static inline bool buttonPressed() { return digitalRead(BUTTON_PIN) == LOW; }

static float adcToPercent(int raw, int center) {
  int d = raw - center;
  if (abs(d) <= deadzone) return 0.0f;
  int span = (4095 / 2) - deadzone;
  float pct = (float)d / (float)span * 100.0f;
  return clampf(pct, -100.0f, 100.0f);
}
static float curveLog(float pct) {
  float x = fabsf(pct) / 100.0f;
  float y = powf(x, 1.5f) * 100.0f;
  return (pct < 0.0f) ? -y : y;
}
static uint8_t flagsByte() {
  uint8_t f = 0;
  if (optInvSpeed) f |= 1 << 0;
  if (optInvSteer) f |= 1 << 1;
  if (optLog) f |= 1 << 2;
  return f;
}

// ===================== ESPNOW =====================
bool initEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(onEspNowSent);
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, PEER_MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) == ESP_OK) peerAdded = true;
  return peerAdded;
}
bool sendPacket(int speedPct, int steerPct) {
  if (!peerAdded) return false;
  ControlPacket p{};
  p.speed_pct = (int16_t)speedPct;
  p.steer_pct = (int16_t)steerPct;
  p.flags = flagsByte();
  p.seq = seq++;
  p.crc = crc16_ccitt((const uint8_t*)&p, sizeof(p) - sizeof(p.crc));
  return esp_now_send(PEER_MAC, (const uint8_t*)&p, sizeof(p)) == ESP_OK;
}

// ===================== Settings =====================
void loadSettings() {
  prefs.begin("joy", true);
  optInvSpeed = prefs.getBool("invSpd", false);
  optInvSteer = prefs.getBool("invStr", false);
  optLog = prefs.getBool("log", true);
  centerX = prefs.getInt("cx", 2048);
  centerY = prefs.getInt("cy", 2048);
  deadzone = prefs.getInt("dz", 180);
  uint8_t stored[6] = {0};
  size_t got = prefs.getBytes("peerMac", stored, 6);
  prefs.end();

  if (got == 6 && memcmp(stored, "\0\0\0\0\0\0", 6) != 0)
    memcpy(PEER_MAC, stored, 6);
  else if (NUM_BOARDS > 0)
    memcpy(PEER_MAC, BOARDS[0].mac, 6);
}
void saveSettings() {
  prefs.begin("joy", false);
  prefs.putBool("invSpd", optInvSpeed);
  prefs.putBool("invStr", optInvSteer);
  prefs.putBool("log", optLog);
  prefs.putInt("cx", centerX);
  prefs.putInt("cy", centerY);
  prefs.putInt("dz", deadzone);
  prefs.end();
}
void savePeerMacToNVS(const uint8_t mac[6]) {
  prefs.begin("joy", false);
  prefs.putBytes("peerMac", mac, 6);
  prefs.end();
}

// ===================== Menu =====================
int menuIndex = 0;
const char* menuName(int i) {
  switch (i) {
    case 0: return "Inv Spd";
    case 1: return "Inv Str";
    case 2: return "Mode:";
    default: return "";
  }
}
void menuToggle(int i) {
  switch (i) {
    case 0: optInvSpeed = !optInvSpeed; break;
    case 1: optInvSteer = !optInvSteer; break;
    case 2: optLog = !optLog; break;
  }
  saveSettings();
}
void drawMenu() {
  screen.clear();
  for (int i = 0; i < 3; i++) {
    int y = 14 + i * 12;
    screen.setCursor(-4, y);
    screen.print(i == menuIndex ? "~ " : " ");
    screen.setCursor(5, y);
    screen.print(menuName(i));
    screen.setCursor(50, y);
    if (i == 2)
      screen.print(optLog ? "Log" : "Lin");
    else
      screen.print((i == 0 ? optInvSpeed : optInvSteer) ? "X" : "O");
  }
  screen.update();
}

// ===================== SELECT =====================
void drawSelect() {
  screen.clear();
  int start = selectIdx - 1;
  if (start < 0) start = 0;
  if (start + 3 > (int)NUM_BOARDS) start = max(0, (int)NUM_BOARDS - 3);
  for (int row = 0; row < 3; ++row) {
    int idx = start + row;
    if (idx >= (int)NUM_BOARDS) break;
    int y = 14 + row * 12;
    screen.setCursor(-4, y);
    screen.print(idx == selectIdx ? "~ " : "  ");
    screen.setCursor(5, y);
    screen.print(BOARDS[idx].name);
  }
  screen.update();
}

// ===================== Setup =====================
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT_X_PIN, INPUT);
  pinMode(POT_Y_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  screen.begin();
  screen.setFont(u8g2_font_6x10_tr);

  screen.clear();
  screen.setCursor(0, 16); screen.print("ESP-NOW Joystick");
  screen.setCursor(0, 28); screen.print("MAC: "); screen.print(WiFi.macAddress().c_str());
  screen.update();
  delay(600);

  loadSettings();

  // Fast calibration
  const uint32_t T = 1000, t0 = millis();
  long sx = 0, sy = 0; int n = 0;
  while (millis() - t0 < T) { sx += analogRead(POT_X_PIN); sy += analogRead(POT_Y_PIN); n++; delay(2); }
  centerX = sx / max(1, n);
  centerY = sy / max(1, n);
  saveSettings();

  initEspNow();

  screen.clear();
  screen.setCursor(0, 14); screen.print("Speed: 0");
  screen.setCursor(0, 26); screen.print("Steer: 0");
  screen.setCursor(0, 38); screen.print("Btn: Up");
  screen.update();
}

// ===================== Loop =====================
void loop() {
  const uint32_t now = millis();
  int rawX = analogRead(POT_X_PIN);
  int rawY = analogRead(POT_Y_PIN);

  bool btn = buttonPressed();
  if (btn && !btnPrev) {
    tBtnDown = now;
    menuShown = false;
    selectShown = false;
  }

  if (btn) {
    unsigned held = now - tBtnDown;

    if (!selectShown && held >= LONG_PRESS_SELECT_MS) {
      ui = SELECT;
      selectIdx = 0;
      drawSelect();
      selectShown = true;
    }
    else if (!menuShown && held >= LONG_PRESS_MENU_MS && ui == RUN) {
      ui = MENU;
      drawMenu();
      menuShown = true;
    }
  }

  if (!btn && btnPrev) {
    if (ui == MENU && !selectShown && menuShown)
      menuToggle(menuIndex);
    else if (ui == SELECT && !selectShown) {
      memcpy(PEER_MAC, BOARDS[selectIdx].mac, 6);
      savePeerMacToNVS(PEER_MAC);
      screen.clear();
      screen.setCursor(0, 20);
      screen.print("Saved. Rebooting");
      screen.update();
      delay(600);
      ESP.restart();
    }
  }
  btnPrev = btn;

  // ----- SELECT MODE -----
  if (ui == SELECT) {
    float xPct = adcToPercent(rawX, centerX);
    if (xPct > 30) { selectIdx = (selectIdx - 1 + (int)NUM_BOARDS) % (int)NUM_BOARDS; delay(180); }
    else if (xPct < -30) { selectIdx = (selectIdx + 1) % (int)NUM_BOARDS; delay(180); }
    drawSelect();
    return;
  }

  // ----- MENU MODE -----
  if (ui == MENU) {
    float xPct = adcToPercent(rawX, centerX);
    if (xPct > 30) { menuIndex = (menuIndex + 2) % 3; delay(180); }
    else if (xPct < -30) { menuIndex = (menuIndex + 1) % 3; delay(180); }
    drawMenu();
    return;
  }

  // ----- RUN MODE -----
  float xPct = adcToPercent(rawX, centerX);
  float yPct = adcToPercent(rawY, centerY);
  if (optLog) { xPct = curveLog(xPct); yPct = curveLog(yPct); }
  int speed = (int)roundf(clampf(-xPct, -100.0f, 100.0f));
  int steer = (int)roundf(clampf(+yPct, -100.0f, 100.0f));
  if (optInvSpeed) speed = -speed;
  if (optInvSteer) steer = -steer;

  if (now - tLastSend >= SEND_MS) {
    tLastSend = now;
    sendPacket(speed, steer);
  }

  if (now - tLastOLED >= OLED_MS) {
    tLastOLED = now;
    screen.clear();
    char line[24];
    screen.setCursor(0, 14);
    snprintf(line, sizeof(line), "Speed:%5d", speed); screen.print(line);
    screen.setCursor(0, 26);
    snprintf(line, sizeof(line), "Steer:%5d", steer); screen.print(line);
    screen.setCursor(0, 38);
    screen.print("Btn: "); screen.print(buttonPressed() ? "Down" : "Up");
    screen.update();
  }
}