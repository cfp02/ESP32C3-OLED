#include "OLEDScreen.h"

OLEDScreen screen;
unsigned long lastScrollTime = 0;
const unsigned long SCROLL_DELAY = 100; // Scroll every 100ms
int scrollOffset = 0;

void setup() {
  delay(1000);
  screen.begin();
  screen.setFont(u8g2_font_6x10_tr);
  
  // Enable vertical scrolling
  screen.enableVerticalScroll(true);
  screen.enableHorizontalScroll(true);
  
  // Add some initial text to scroll
  screen.scrollText("Welcome to the scrolling demo!", 0);
  screen.scrollText("This is a vertical scrolling test.", 0);
  screen.scrollText("You can scroll both vertically and horizontally.", 0);
  screen.scrollText("Try adding more text to see the scrolling effect.", 0);
  screen.scrollText("The text will scroll up as new lines are added.", 0);
  
  // Start with horizontal scroll
  screen.scrollTextHorizontal("This is a very long text that will scroll horizontally across the screen...");
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastScrollTime >= SCROLL_DELAY) {
    // Update horizontal scroll position
    scrollOffset = (scrollOffset + 1) % 300; // Adjust this value based on your text length
    screen.setScrollOffset(scrollOffset, 0);
    screen.scrollTextHorizontal("This is a very long text that will scroll horizontally across the screen...");
    
    lastScrollTime = currentTime;
  }
  
  // Add new text every 5 seconds to demonstrate vertical scrolling
  if (currentTime % 5000 == 0) {
    char newText[72];
    sprintf(newText, "New line %d added to the scroll buffer!", (currentTime / 5000));
    screen.scrollText(newText, 0);
    screen.scrollTextVertical(newText);
  }
}
 