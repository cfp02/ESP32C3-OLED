#include <Arduino.h>
#include "OLEDScreen.h"

#define BUTTON_PIN GPIO_NUM_0
#define POT_X_PIN GPIO_NUM_2
#define POT_Y_PIN GPIO_NUM_1
#define LED_PIN GPIO_NUM_8

// Create OLED screen instance
OLEDScreen screen;

// Potentiometer values
int potX = 0;
int potY = 0;
int potX_pct = 0;
int potY_pct = 0;
int buttonState = 0;

// Update interval for potentiometer readings (in milliseconds)
const unsigned long UPDATE_INTERVAL = 100;

// Last update time
unsigned long lastUpdate = 0;

// Buffer for formatted strings
char displayBuffer[20];

void setup() {
    // Initialize serial for debugging
    // Serial.begin(115200);
    
    // Initialize OLED screen
    screen.begin();
    screen.setFont(u8g2_font_6x10_tr);
    
    // Set up analog pins
    pinMode(POT_X_PIN, INPUT);  // X potentiometer
    pinMode(POT_Y_PIN, INPUT);  // Y potentiometer
    
    // Set up switch pin with internal pull-up
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Switch with internal pull-up
    
    // Set up LED pin
    pinMode(LED_PIN, OUTPUT);  // LED on IO8
    digitalWrite(LED_PIN, HIGH);  // Start with LED off

    const char* lineTexts[] = {
    "Some text that can scroll",
    "Some other text that can scroll",
    "Line three scrolls",
    "Line four scrolls"
    };

    screen.enableHorizontalScroll(true);
}

void loop() {
    unsigned long currentTime = millis();
    
    // Check if it's time to update
    if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
        // Read potentiometer values
        potX = analogRead(POT_X_PIN);
        potY = analogRead(POT_Y_PIN);
        potX_pct = map(potX, 0, 4095, -100, 100);
        potY_pct = map(potY, 0, 4095, -100, 100);
        buttonState = !digitalRead(BUTTON_PIN);
        
        // Update LED based on button state
        digitalWrite(LED_PIN, !buttonState);
        
        // Update display
        screen.clear();
        
        // Format and display X value
        snprintf(displayBuffer, sizeof(displayBuffer), "X: %d", potX_pct);
        screen.setCursor(0, 17);
        screen.print(displayBuffer);
        
        // Format and display Y value
        snprintf(displayBuffer, sizeof(displayBuffer), "Y: %d", potY_pct);
        screen.setCursor(0, 27);
        screen.print(displayBuffer);
        
        // Format and display button state
        snprintf(displayBuffer, sizeof(displayBuffer), "Btn: %s", buttonState ? "Up" : "Down");
        screen.setCursor(0, 37);
        screen.print(displayBuffer);
        
        screen.update();
        
        // Update the last update time
        lastUpdate = currentTime;
    }
}
 