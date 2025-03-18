#include "OLEDScreen.h"

OLEDScreen screen;
unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 20;  // Update scroll position every 200ms (slower)

// Sample text for each line
const char* lineTexts[] = {
    "Some text that can scroll",
    "Some other text that can scroll",
    "Line three scrolls",
    "Line four scrolls"
};

void setup() {
  delay(1000);
  screen.begin();
  screen.setFont(u8g2_font_6x10_tr);
  
  // Enable horizontal scrolling
  screen.enableHorizontalScroll(true);
  
  // Initialize all lines with their text
  for (int i = 0; i < 4; i++) {
    screen.scrollTextHorizontal(lineTexts[i], i);
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update scroll positions every UPDATE_INTERVAL
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    // Update all lines at once
    screen.updateAllLines();
    lastUpdateTime = currentTime;
  }
}
 