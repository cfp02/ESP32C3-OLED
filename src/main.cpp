#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>
#include "OLEDScreen.h"

// ----------------- Pins (ESP32-C3) -----------------
#define BUTTON_PIN  GPIO_NUM_0  // joystick push (active LOW)
#define POT_X_PIN   GPIO_NUM_2  // X axis
#define POT_Y_PIN   GPIO_NUM_1  // Y axis
#define LED_PIN     GPIO_NUM_8  // keep OFF

// ----------------- ESPNOW peer ----------------
static uint8_t PEER_MAC[6] = { 0x24,0x6F,0x28,0x00,0x00,0x00 };  // <-- set to receiver MAC
static const uint8_t ESPNOW_CHANNEL = 6; // not forced with low-level calls

// ----------------- Packet ---------------------
struct __attribute__((packed)) ControlPacket {
  int16_t speed_pct;   // -100..+100 (throttle)
  int16_t steer_pct;   // -100..+100 (left positive)
  uint8_t flags;       // bit0=invSpeed, bit1=invSteer, bit2=logMode
  uint8_t seq;
  uint16_t crc;        // CRC16-CCITT over bytes 0..4
};

static uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc=0xFFFF){
  while (len--){
    crc ^= ((uint16_t)*data++) << 8;
    for (int i=0;i<8;i++) crc = (crc & 0x8000) ? (crc<<1) ^ 0x1021 : (crc<<1);
  }
  return crc;
}

// ----------------- Globals --------------------
OLEDScreen screen;
Preferences prefs;

enum UiState { RUN, MENU };
UiState ui = RUN;

int centerX = 2048, centerY = 2048;   // learned at boot
int deadzone = 180;                    // ~4–5%

// Settings (persist)
bool optInvSpeed  = false;  // invert final speed sign
bool optInvSteer  = false;  // invert final steer sign
bool optLog       = true;   // Log curve vs Linear

// timing
uint8_t  seq = 0;
uint32_t tLastSend = 0;
uint32_t tLastOLED = 0;
const unsigned SEND_MS = 20;           // 50 Hz
const unsigned OLED_RUN_MS = 100;      // 10 Hz display in RUN

// button edges / long press
bool btnPrev = false;
bool longPressHandled = false;
uint32_t tBtnDown = 0;
const unsigned LONG_PRESS_MS = 5000;

// ----------------- Small helpers --------------
static inline float clampf(float x, float a, float b){ return x < a ? a : (x > b ? b : x); }
static inline bool  buttonPressed(){ return digitalRead(BUTTON_PIN) == LOW; }

static float adcToPercent(int raw, int center){
  int d = raw - center;
  if (abs(d) <= deadzone) return 0.0f;
  int span = (4095/2) - deadzone;
  float pct = (float)d / (float)span * 100.0f;
  return clampf(pct, -100.0f, 100.0f);
}

// Log-ish response for finer control near center
static float curveLog(float pct){
  float x = fabsf(pct) / 100.0f;
  float y = powf(x, 1.5f) * 100.0f;   // tweak exponent if needed
  return (pct < 0.0f) ? -y : y;
}

static uint8_t flagsByte(){
  uint8_t f = 0;
  if (optInvSpeed)  f |= (1<<0);
  if (optInvSteer)  f |= (1<<1);
  if (optLog)       f |= (1<<2);
  return f;
}

// ----------------- ESPNOW ---------------------
static bool peerAdded = false;
void onEspNowSent(const uint8_t*, esp_now_send_status_t){ /* no LED blink */ }

bool initEspNow(){
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // optional
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(onEspNowSent);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, PEER_MAC, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) == ESP_OK) peerAdded = true;
  return peerAdded;
}

bool sendPacket(int speedPct, int steerPct){
  if (!peerAdded) return false;
  ControlPacket p{};
  p.speed_pct = (int16_t)speedPct;
  p.steer_pct = (int16_t)steerPct;
  p.flags     = flagsByte();
  p.seq       = seq++;
  p.crc       = 0;
  p.crc       = crc16_ccitt((const uint8_t*)&p, sizeof(p)-sizeof(p.crc));
  return esp_now_send(PEER_MAC, (const uint8_t*)&p, sizeof(p)) == ESP_OK;
}

// ----------------- Settings store --------------
void loadSettings(){
  prefs.begin("joy", true);
  optInvSpeed = prefs.getBool("invSpd", false);
  optInvSteer = prefs.getBool("invStr", false);
  optLog      = prefs.getBool("log",    true);
  centerX     = prefs.getInt ("cx",     2048);
  centerY     = prefs.getInt ("cy",     2048);
  deadzone    = prefs.getInt ("dz",     180);
  prefs.end();
}
void saveSettings(){
  prefs.begin("joy", false);
  prefs.putBool("invSpd", optInvSpeed);
  prefs.putBool("invStr", optInvSteer);
  prefs.putBool("log",    optLog);
  prefs.putInt ("cx",     centerX);
  prefs.putInt ("cy",     centerY);
  prefs.putInt ("dz",     deadzone);
  prefs.end();
}

// ----------------- Menu UI --------------------
int menuIndex = 0; // 0..2
const char* menuName(int i){
  switch(i){
    case 0: return "Inv Spd";
    case 1: return "Inv Str";
    case 2: return "Mode:";
    default: return "";
  }
}
void menuToggle(int i){
  switch(i){
    case 0: optInvSpeed = !optInvSpeed; break;
    case 1: optInvSteer = !optInvSteer; break;
    case 2: optLog      = !optLog;      break;
  }
  saveSettings();
}
void drawMenu(){
  // Three lines only, no header
  screen.clear();
  for (int i=0;i<3;i++){
    int y = 14 + i*12;                // 16, 28, 40
    // marker
    screen.setCursor(-4, y);
    screen.print(i==menuIndex ? "~ " : " ");
    // name
    screen.setCursor(5, y);
    screen.print(menuName(i));
    // value
    screen.setCursor(50, y);
    if (i == 2) {                     // Mode
      screen.print(optLog ? "Log" : "Lin");
    } else {                          // Inv flags
      screen.print((i==0 ? optInvSpeed : optInvSteer) ? "X" : "O");
    }
  }
  screen.update();
}

// ----------------- Setup/Loop -----------------
void setup(){
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT_X_PIN,  INPUT);
  pinMode(POT_Y_PIN,  INPUT);
  pinMode(LED_PIN,    OUTPUT);
  digitalWrite(LED_PIN, HIGH); // keep OFF

  Serial.begin(115200);
  delay(50);

  screen.begin();
  screen.setFont(u8g2_font_6x10_tr);

  // Show MAC briefly to help pairing
  screen.clear();
  screen.setCursor(0, 16); screen.print("ESP-NOW Joystick");
  screen.setCursor(0, 28); screen.print("MAC: "); screen.print(WiFi.macAddress().c_str());
  screen.update();
  delay(600);

  loadSettings();

  // quick auto-calibration (~1s)
  {
    const uint32_t T=1000, t0=millis();
    long sx=0, sy=0; int n=0;
    while (millis()-t0 < T){ sx += analogRead(POT_X_PIN); sy += analogRead(POT_Y_PIN); n++; delay(2); }
    centerX = sx / max(1,n);
    centerY = sy / max(1,n);
    saveSettings();
  }

  if (!initEspNow()){
    screen.clear();
    screen.setCursor(0, 22); screen.print("ESPNOW init FAIL");
    screen.setCursor(0, 34); screen.print("Check MAC/channel");
    screen.update();
    // continue anyway so UI still works
  }

  // Initial RUN screen (exactly 3 lines)
  screen.clear();
  screen.setCursor(0, 14); screen.print("Speed: 0");
  screen.setCursor(0, 26); screen.print("Steer: 0");
  screen.setCursor(0, 38); screen.print("Btn: Up");
  screen.update();
}

void loop(){
  const uint32_t now = millis();

  // read inputs
  int rawX = analogRead(POT_X_PIN);
  int rawY = analogRead(POT_Y_PIN);

  // button edges / long press
  bool btn = buttonPressed();
  if (btn && !btnPrev) {
    tBtnDown = now;
    longPressHandled = false;         // new press → re-arm long press
  }
  if (btn && !longPressHandled && (now - tBtnDown >= LONG_PRESS_MS)) {
    // long-press: toggle screens ONCE, then require release
    ui = (ui == RUN) ? MENU : RUN;
    longPressHandled = true;
  }
  if (!btn && btnPrev) {
    // button released
    if (ui == MENU && !longPressHandled) {
      // short press in MENU toggles selected option
      menuToggle(menuIndex);
    }
    longPressHandled = false;
  }
  btnPrev = btn;

  if (ui == MENU){
    // Navigate menu with X (left/right)
    float xPct = adcToPercent(rawX, centerX);
    if (xPct > 30)      { menuIndex = (menuIndex + 2) % 3; delay(180); } // left
    else if (xPct < -30){ menuIndex = (menuIndex + 1) % 3; delay(180); } // right
    drawMenu();
    return;
  }

  // ---- RUN mode ----
  // Convert ADC to percents
  float xPct = adcToPercent(rawX, centerX);
  float yPct = adcToPercent(rawY, centerY);

  // Apply curve
  if (optLog){ xPct = curveLog(xPct); yPct = curveLog(yPct); }

  // Your mapping:
  // +speed = -X ;  +steer = +Y
  int speed = (int)roundf(clampf(-xPct, -100.0f, 100.0f));
  int steer = (int)roundf(clampf(+yPct, -100.0f, 100.0f));

  // Apply user inversions (final)
  if (optInvSpeed) speed = -speed;
  if (optInvSteer) steer = -steer;

  // Send ESPNOW at fixed rate
  if (now - tLastSend >= SEND_MS){
    tLastSend = now;
    sendPacket(speed, steer);
    // keep LED off
  }

  // OLED: exactly three lines (speed, steer, button)
  if (now - tLastOLED >= OLED_RUN_MS){
    tLastOLED = now;
    screen.clear();

    char line[24];
    screen.setCursor(0, 14);
    snprintf(line, sizeof(line), "Speed:%5d", speed);
    screen.print(line);

    screen.setCursor(0, 26);
    snprintf(line, sizeof(line), "Steer:%5d", steer);
    screen.print(line);

    screen.setCursor(0, 38);
    screen.print("Btn: ");
    screen.print(buttonPressed() ? "Down" : "Up");

    screen.update();
  }
}