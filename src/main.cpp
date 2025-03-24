#include <Arduino.h>
#include "DisplayManager.h"
#include "EncoderManager.h"
#include "InputHandler.h"

// Pin definitions
const uint8_t JOYSTICK_X_PIN = GPIO_NUM_0;
const uint8_t JOYSTICK_Y_PIN = GPIO_NUM_1;
const uint8_t JOYSTICK_BUTTON_PIN = GPIO_NUM_2;
const uint8_t LED_PIN = GPIO_NUM_8;

// Update intervals
const unsigned long DISPLAY_UPDATE_INTERVAL = 50;  // 50ms = 20Hz update rate
unsigned long lastDisplayUpdate = 0;

// Create instances
DisplayManager display;
EncoderManager encoder;
InputHandler inputHandler(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_BUTTON_PIN);

void updateEncoderDisplay() {
    display.clear();
    
    // Display encoder values
    float degrees = encoder.getDegrees();
    word raw = encoder.getRawAngle();
    
    // Show angle in degrees
    display.setCursor(0, 10);
    display.printf("Angle: %.1f°", degrees);
    
    // Show raw value
    display.setCursor(0, 25);
    display.printf("Raw: %d", raw);
    
    // Show magnet status
    display.setCursor(0, 40);
    if (!encoder.isMagnetDetected()) {
        display.print("No Magnet!");
    } else {
        int magnitude = encoder.getMagnitude();
        display.printf("Mag: %d", magnitude);
    }
    
    display.update();
}

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(1000);  // Give serial time to initialize
    Serial.println("\nStarting AS5600 Encoder Test");
    
    // Configure LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // Active LOW
    
    // Initialize input handler
    inputHandler.begin();
    
    // Initialize display
    Serial.println("Initializing display...");
    display.begin();
    display.setOrientation(OLEDScreen::Orientation::HORIZONTAL);
    
    // Show connecting message
    display.clear();
    display.setCursor(0, 25);
    display.print("Connecting to");
    display.setCursor(0, 35);
    display.print("AS5600...");
    display.update();
    delay(1000);  // Show message for 1 second
    
    // Initialize encoder
    Serial.println("Initializing AS5600 encoder...");
    if (!encoder.begin()) {
        Serial.println("Failed to detect AS5600 encoder!");
        display.clear();
        display.setCursor(0, 25);
        display.print("No Magnet!");
        display.update();
        while(1) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(500);  // Blink LED to indicate error
        }
    }
    Serial.println("AS5600 encoder initialized successfully");
    
    // Initial display update
    updateEncoderDisplay();
}

void loop() {
    // Update input handler
    inputHandler.update();
    
    // Handle LED state based on current button state
    digitalWrite(LED_PIN, !inputHandler.getButtonState());  // Active LOW
    
    // Update display at regular intervals
    unsigned long currentMillis = millis();
    if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = currentMillis;
        updateEncoderDisplay();
    }
}
 