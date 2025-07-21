#include <Arduino.h>
#include "OLEDScreen.h"
#include "BLEHandler.h"

// Left joystick pins
#define JOYSTICK1_X_PIN GPIO_NUM_4  // Left joystick X axis
#define JOYSTICK1_Y_PIN GPIO_NUM_3  // Left joystick Y axis
#define JOYSTICK1_BUTTON_PIN GPIO_NUM_10  // Left joystick button (changed from GPIO5)

// Right joystick pins
#define JOYSTICK2_X_PIN GPIO_NUM_1  // Right joystick X axis
#define JOYSTICK2_Y_PIN GPIO_NUM_0  // Right joystick Y axis
#define JOYSTICK2_BUTTON_PIN GPIO_NUM_2  // Right joystick button

// LED pin
#define LED_PIN GPIO_NUM_8

// Axis mapping configuration
// Choose which axes to use for motor control
// Options: JOYSTICK1_X, JOYSTICK1_Y, JOYSTICK2_X, JOYSTICK2_Y
#define RIGHT_MOTOR_AXIS JOYSTICK1_Y  // Axis for right motor (forward/backward)
#define LEFT_MOTOR_AXIS JOYSTICK1_X   // Axis for left motor (steering)

// Axis constants for mapping
#define JOYSTICK1_X 1
#define JOYSTICK1_Y 2
#define JOYSTICK2_X 3
#define JOYSTICK2_Y 4

// Long press duration (5 seconds)
#define LONG_PRESS_DURATION 5000

// Create OLED screen instance
OLEDScreen screen;

// Create BLE handler instance
BLEHandler bleHandler;

// Joystick values
int joystick1X = 0;
int joystick1Y = 0;
int joystick1X_pct = 0;
int joystick1Y_pct = 0;
int joystick1ButtonState = 0;

int joystick2X = 0;
int joystick2Y = 0;
int joystick2X_pct = 0;
int joystick2Y_pct = 0;
int joystick2ButtonState = 0;

        // Motor power values (mapped from selected axes)
        int leftMotorPower = 0;
        int rightMotorPower = 0;

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
    Serial.println("Starting OLED initialization...");
    screen.begin();
    Serial.println("OLED begin() completed");
    screen.setFont(u8g2_font_6x10_tr);
    Serial.println("OLED font set");
    Serial.println("OLED Screen initialized");
    
    // Initialize BLE
    bleHandler.begin();
    Serial.println("BLE initialized as ble-joystick");
    
    // Set up joystick 1 pins
    pinMode(JOYSTICK1_X_PIN, INPUT);  // Joystick 1 X axis
    pinMode(JOYSTICK1_Y_PIN, INPUT);  // Joystick 1 Y axis
    pinMode(JOYSTICK1_BUTTON_PIN, INPUT_PULLUP);  // Joystick 1 button
    Serial.println("Joystick 1 pins configured");
    
    // Set up joystick 2 pins
    pinMode(JOYSTICK2_X_PIN, INPUT);  // Joystick 2 X axis
    pinMode(JOYSTICK2_Y_PIN, INPUT);  // Joystick 2 Y axis
    pinMode(JOYSTICK2_BUTTON_PIN, INPUT_PULLUP);  // Joystick 2 button
    Serial.println("Joystick 2 pins configured");
    
    // Set up LED pin
    pinMode(LED_PIN, OUTPUT);  // LED on IO8
    digitalWrite(LED_PIN, HIGH);  // Start with LED off
    Serial.println("LED pin configured");


    Serial.println("Setup complete - starting BLE advertising");
    bleHandler.startAdvertising();
}

void handleButtonPresses() {
    // Read joystick 1 button
    joystick1ButtonState = !digitalRead(JOYSTICK1_BUTTON_PIN); // Inverted due to pull-up
    
    // Read joystick 2 button
    joystick2ButtonState = !digitalRead(JOYSTICK2_BUTTON_PIN); // Inverted due to pull-up
    
    // Long press detection on joystick 1 button (5 seconds)
    if (joystick1ButtonState && !buttonPressed) {
        buttonPressed = true;
        buttonPressStart = millis();
        longPressTriggered = false;
    }
    else if (!joystick1ButtonState && buttonPressed) {
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
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle button press detection
    handleButtonPresses();
    
    // Update BLE handler
    bleHandler.update();
    
    // Check if it's time to update
    if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
        // Read joystick 1 values
        joystick1X = analogRead(JOYSTICK1_X_PIN);
        joystick1Y = analogRead(JOYSTICK1_Y_PIN);
        joystick1X_pct = map(joystick1X, 0, 4095, -100, 100);
        joystick1Y_pct = map(joystick1Y, 0, 4095, -100, 100);
        
        // Read joystick 2 values
        joystick2X = analogRead(JOYSTICK2_X_PIN);
        joystick2Y = analogRead(JOYSTICK2_Y_PIN);
        joystick2X_pct = map(joystick2X, 0, 4095, -100, 100);
        joystick2Y_pct = map(joystick2Y, 0, 4095, -100, 100);
        
        // Map selected axes to motor powers
        if (RIGHT_MOTOR_AXIS == JOYSTICK1_Y) {
            rightMotorPower = joystick1Y_pct;
        } else if (RIGHT_MOTOR_AXIS == JOYSTICK1_X) {
            rightMotorPower = joystick1X_pct;
        } else if (RIGHT_MOTOR_AXIS == JOYSTICK2_Y) {
            rightMotorPower = joystick2Y_pct;
        } else if (RIGHT_MOTOR_AXIS == JOYSTICK2_X) {
            rightMotorPower = joystick2X_pct;
        }
        
        if (LEFT_MOTOR_AXIS == JOYSTICK1_Y) {
            leftMotorPower = joystick1Y_pct;
        } else if (LEFT_MOTOR_AXIS == JOYSTICK1_X) {
            leftMotorPower = joystick1X_pct;
        } else if (LEFT_MOTOR_AXIS == JOYSTICK2_Y) {
            leftMotorPower = joystick2Y_pct;
        } else if (LEFT_MOTOR_AXIS == JOYSTICK2_X) {
            leftMotorPower = joystick2X_pct;
        }
        
        // Send motor power data if connected
        if (bleHandler.isConnected()) {
            bleHandler.sendJoystickData(leftMotorPower, rightMotorPower);
        }
        
        // Update LED based on button state and BLE connection
        if (bleHandler.isConnected()) {
            digitalWrite(LED_PIN, LOW); // Solid LED when connected
        } else {
            digitalWrite(LED_PIN, !joystick1ButtonState);
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
        
        // Create strings beforehand with fixed-width formatting
        char leftString[20];
        char rightString[20];
        
        snprintf(leftString, sizeof(leftString), "%4d:%4d %s", joystick1X_pct, joystick1Y_pct, joystick1ButtonState ? "D" : "U");
        snprintf(rightString, sizeof(rightString), "%4d:%4d %s", joystick2X_pct, joystick2Y_pct, joystick2ButtonState ? "D" : "U");
        
        // Debug: print to serial what we're trying to display
        Serial.printf("Left: '%s', Right: '%s'\n", leftString, rightString);
        
        // Display left joystick (top row): "x:y btn"
        screen.setCursor(0, 15);
        screen.print(leftString);
        
        // Display right joystick (bottom row): "x:y btn"
        screen.setCursor(0, 30);
        screen.print(rightString);
        
        screen.update();
        
        // Update the last update time
        lastUpdate = currentTime;
    }
}