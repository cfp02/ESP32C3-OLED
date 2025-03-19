#include <Arduino.h>
#include "DisplayManager.h"
#include "TaskManager.h"

// Pin definitions
const uint8_t JOYSTICK_X_PIN = GPIO_NUM_0;
const uint8_t JOYSTICK_Y_PIN = GPIO_NUM_1;
const uint8_t JOYSTICK_BUTTON_PIN = GPIO_NUM_2;
const uint8_t LED_PIN = GPIO_NUM_8;

// Joystick thresholds (in percentage)
const int8_t DEADBAND_THRESHOLD = 20;    // 20% deadband in the middle
const int8_t TOGGLE_THRESHOLD = 50;      // 50% threshold for toggle
const int8_t NAVIGATION_THRESHOLD = 30;  // 30% threshold for navigation

// Joystick state
int8_t lastXValue = 0;
int8_t lastYValue = 0;
bool lastButtonState = false;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Navigation state
bool wasInNavigationThreshold = false;
bool wasInToggleThreshold = false;

// Create instances
DisplayManager display;
TaskManager taskManager;

// Map analog value to -100 to 100 range
int8_t mapJoystickValue(int rawValue) {
    // Map from 0-4095 to -100 to 100
    return map(rawValue, 0, 4095, -100, 100);
}

// Check if value is within deadband
bool isInDeadband(int8_t value) {
    return abs(value) < DEADBAND_THRESHOLD;
}

// Check if value exceeds toggle threshold
bool exceedsToggleThreshold(int8_t value) {
    return abs(value) > TOGGLE_THRESHOLD;
}

// Check if value exceeds navigation threshold
bool exceedsNavigationThreshold(int8_t value) {
    return abs(value) > NAVIGATION_THRESHOLD;
}

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    
    // Configure pins
    pinMode(JOYSTICK_X_PIN, INPUT);
    pinMode(JOYSTICK_Y_PIN, INPUT);
    pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    
    // Initialize LED to off state
    digitalWrite(LED_PIN, HIGH);  // Active LOW
    
    // Initialize display
    display.begin();
    
    // Set to horizontal orientation (optional since it's default)
    display.setOrientation(OLEDScreen::Orientation::VERTICAL);
    
    // Add some sample tasks
    taskManager.addTask("Brush", TaskFrequency::DAILY);
    taskManager.addTask("Shave", TaskFrequency::EVERY_2_DAYS);
    taskManager.addTask("Exercise", TaskFrequency::DAILY);
    taskManager.addTask("Floss", TaskFrequency::DAILY);
    taskManager.addTask("Shower", TaskFrequency::DAILY);
    
    // Set all tasks as completed initially
    Serial.println("Setting all tasks as completed:");
    for (uint8_t i = 0; i < taskManager.getTaskCount(); i++) {
        if (!taskManager.isTaskCompleted(i)) {
            taskManager.toggleTask(i);
        }
    }
    
    // Initial display update
    display.displayTaskList(taskManager);
}

void loop() {
    // Read joystick values and map to -100 to 100 range
    int8_t xValue = mapJoystickValue(analogRead(JOYSTICK_X_PIN));
    int8_t yValue = mapJoystickValue(analogRead(JOYSTICK_Y_PIN));
    bool buttonState = !digitalRead(JOYSTICK_BUTTON_PIN);
    
    // Update LED based on button state
    digitalWrite(LED_PIN, !buttonState);  // Active LOW
    
    // Handle button press with debouncing
    if (buttonState != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (buttonState && !lastButtonState) {
            // Button press detected - toggle current task
            taskManager.toggleTask(taskManager.getSelectedTask());
            display.displayTaskList(taskManager);
        }
    }
    
    // Handle joystick movement - Event-based navigation
    bool inNavigationThreshold = exceedsNavigationThreshold(yValue);
    bool inToggleThreshold = exceedsToggleThreshold(xValue);
    
    // Navigation event detection
    if (inNavigationThreshold && !wasInNavigationThreshold) {
        // Just entered threshold, do nothing
    } else if (!inNavigationThreshold && wasInNavigationThreshold) {
        // Left threshold, check direction and move
        if (lastYValue > 0) {
            taskManager.moveSelection(-1);  // Move up
        } else if (lastYValue < 0) {
            taskManager.moveSelection(1);   // Move down
        }
        display.displayTaskList(taskManager);
    }
    
    // Toggle event detection
    if (inToggleThreshold && !wasInToggleThreshold) {
        // Just entered threshold, do nothing
        Serial.println("Entered toggle threshold");
    } else if (!inToggleThreshold && wasInToggleThreshold) {
        // Left threshold, check direction and toggle
        Serial.print("Left toggle threshold, lastXValue = ");
        Serial.println(lastXValue);
        // Toggle on either left or right movement
        if (abs(lastXValue) > TOGGLE_THRESHOLD) {
            Serial.print("Toggling task ");
            Serial.println(taskManager.getSelectedTask());
            taskManager.toggleTask(taskManager.getSelectedTask());
            display.displayTaskList(taskManager);
        }
    }
    
    // Update state tracking
    wasInNavigationThreshold = inNavigationThreshold;
    wasInToggleThreshold = inToggleThreshold;
    lastXValue = xValue;
    lastYValue = yValue;
    lastButtonState = buttonState;
    
    // Small delay to prevent too frequent updates
    delay(50);
}
 