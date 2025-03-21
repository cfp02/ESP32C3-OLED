#include <Arduino.h>
#include "DisplayManager.h"
#include "TaskManager.h"
#include "InputHandler.h"

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
InputHandler inputHandler(JOYSTICK_X_PIN, JOYSTICK_Y_PIN, JOYSTICK_BUTTON_PIN);

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
    
    // Configure LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // Active LOW
    
    // Initialize input handler
    inputHandler.begin();
    
    // Initialize display
    display.begin();
    display.setOrientation(OLEDScreen::Orientation::VERTICAL);
    
    // Try to load tasks from EEPROM
    Serial.println("Loading tasks from EEPROM...");
    if (taskManager.getTaskCount() == 0) {
        // No tasks loaded from EEPROM, add default tasks
        Serial.println("No tasks found in EEPROM, adding defaults:");
        taskManager.addTask("Brush Teeth", TaskFrequency::DAILY);
        taskManager.addTask("Shave", TaskFrequency::EVERY_2_DAYS);
        taskManager.addTask("Exercise", TaskFrequency::DAILY);
        taskManager.addTask("Floss", TaskFrequency::DAILY);
        taskManager.addTask("Shower", TaskFrequency::DAILY);
    } else {
        Serial.print("Loaded ");
        Serial.print(taskManager.getTaskCount());
        Serial.println(" tasks from EEPROM");
    }
    
    // Initial display update
    display.displayTaskList(taskManager);
}

void loop() {
    // Update input handler
    inputHandler.update();
    
    // Handle LED state based on current button state
    digitalWrite(LED_PIN, !inputHandler.getButtonState());  // Active LOW
    
    bool needsDisplayUpdate = false;
    
    // Handle vertical movement
    switch (inputHandler.getVerticalMovement()) {
        case InputHandler::Movement::UP:
            taskManager.moveSelection(-1);
            needsDisplayUpdate = true;
            break;
        case InputHandler::Movement::DOWN:
            taskManager.moveSelection(1);
            needsDisplayUpdate = true;
            break;
        default:
            break;
    }
    
    // Handle toggle requests (from either button or joystick)
    if (inputHandler.getToggleRequested() || inputHandler.getButtonPressed()) {
        Serial.print("Toggling task ");
        Serial.println(taskManager.getSelectedTask());
        taskManager.toggleTask(taskManager.getSelectedTask());
        needsDisplayUpdate = true;
    }
    
    // Update display if needed
    if (needsDisplayUpdate) {
        display.displayTaskList(taskManager);
    }
    
    // Small delay to prevent too frequent updates
    delay(50);
}
 