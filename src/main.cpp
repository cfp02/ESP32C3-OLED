#include <Arduino.h>
#include "OLEDScreen.h"
#include "BLEHandler.h"

#define BUTTON_PIN GPIO_NUM_0
#define POT_X_PIN GPIO_NUM_2
#define POT_Y_PIN GPIO_NUM_1
#define LED_PIN GPIO_NUM_8

// Long press duration (5 seconds)
#define LONG_PRESS_DURATION 5000

// Create OLED screen instance
OLEDScreen screen;

// Create BLE handler instance
BLEHandler bleHandler;

// Potentiometer values
int potX = 0;
int potY = 0;
int potX_pct = 0;
int potY_pct = 0;
int buttonState = 0;

// Button press tracking
unsigned long buttonPressStart = 0;
bool buttonPressed = false;
bool longPressTriggered = false;

// Update interval for potentiometer readings (in milliseconds)
const unsigned long UPDATE_INTERVAL = 100;

// Last update time
unsigned long lastUpdate = 0;

// Buffer for formatted strings
char displayBuffer[20];

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    Serial.println("ESP32C3 BLE Joystick Controller Starting...");
    
    // Initialize OLED screen
    screen.begin();
    screen.setFont(u8g2_font_6x10_tr);
    Serial.println("OLED Screen initialized");
    
    // Initialize BLE
    bleHandler.begin();
    Serial.println("BLE initialized as ble-joystick");
    
    // Set up analog pins
    pinMode(POT_X_PIN, INPUT);  // X potentiometer
    pinMode(POT_Y_PIN, INPUT);  // Y potentiometer
    Serial.println("Analog pins configured");
    
    // Set up switch pin with internal pull-up
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Switch with internal pull-up
    Serial.println("Button pin configured");
    
    // Set up LED pin
    pinMode(LED_PIN, OUTPUT);  // LED on IO8
    digitalWrite(LED_PIN, HIGH);  // Start with LED off
    Serial.println("LED pin configured");

    const char* lineTexts[] = {
    "Some text that can scroll",
    "Some other text that can scroll",
    "Line three scrolls",
    "Line four scrolls"
    };

    screen.enableHorizontalScroll(true);
    Serial.println("Setup complete - starting BLE advertising");
    bleHandler.startAdvertising();
}

void handleButtonPress() {
    bool currentButtonState = !digitalRead(BUTTON_PIN); // Inverted due to pull-up
    
    // Button press detection
    if (currentButtonState && !buttonPressed) {
        buttonPressed = true;
        buttonPressStart = millis();
        longPressTriggered = false;
    }
    else if (!currentButtonState && buttonPressed) {
        buttonPressed = false;
        buttonPressStart = 0;
    }
    
    // Long press detection (5 seconds)
    if (buttonPressed && !longPressTriggered && 
        (millis() - buttonPressStart >= LONG_PRESS_DURATION)) {
        longPressTriggered = true;
        
        Serial.println("Long press detected - restarting BLE advertising");
        
        // Restart BLE advertising
        bleHandler.startAdvertising();
        
        // Visual feedback - blink LED rapidly
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_PIN, LOW);
            delay(100);
            digitalWrite(LED_PIN, HIGH);
            delay(100);
        }
    }
    
    buttonState = currentButtonState;
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle button press detection
    handleButtonPress();
    
    // Update BLE handler
    bleHandler.update();
    
    // Check if it's time to update
    if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
        // Read potentiometer values
        potX = analogRead(POT_X_PIN);
        potY = analogRead(POT_Y_PIN);
        potX_pct = map(potX, 0, 4095, -100, 100);
        potY_pct = map(potY, 0, 4095, -100, 100);
        
        // Send joystick data to car if connected
        if (bleHandler.isConnected()) {
            bleHandler.sendJoystickData(potX_pct, potY_pct);
        }
        
        // Update LED based on button state and BLE connection
        if (bleHandler.isConnected()) {
            digitalWrite(LED_PIN, LOW); // Solid LED when connected
        } else {
            digitalWrite(LED_PIN, !buttonState);
        }
        
        // Print BLE status to Serial every few seconds
        static unsigned long lastStatusPrint = 0;
        static bool lastConnectionState = false;
        if (currentTime - lastStatusPrint >= 3000) { // Every 3 seconds
            lastStatusPrint = currentTime;
            if (bleHandler.isAdvertising()) {
                Serial.println("Status: BLE advertising - waiting for connection");
            } else if (bleHandler.isConnected()) {
                Serial.println("Status: BLE connected");
                // Print device details when connection state changes
                if (!lastConnectionState) {
                    bleHandler.printConnectedDeviceInfo();
                }
            } else {
                Serial.println("Status: BLE disconnected");
            }
            lastConnectionState = bleHandler.isConnected();
        }
        
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
        snprintf(displayBuffer, sizeof(displayBuffer), "Btn: %s", buttonState ? "Down" : "Up");
        screen.setCursor(0, 37);
        screen.print(displayBuffer);
        
        // Display BLE status
        if (bleHandler.isAdvertising()) {
            snprintf(displayBuffer, sizeof(displayBuffer), "BLE: Advertising");
        } else if (bleHandler.isConnected()) {
            snprintf(displayBuffer, sizeof(displayBuffer), "BLE: Connected");
        } else {
            snprintf(displayBuffer, sizeof(displayBuffer), "BLE: Disconnected");
        }
        screen.setCursor(0, 47);
        screen.print(displayBuffer);
        
        screen.update();
        
        // Update the last update time
        lastUpdate = currentTime;
    }
}