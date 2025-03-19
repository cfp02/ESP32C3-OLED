#pragma once

#include <Arduino.h>

class InputHandler {
public:
    // Movement states
    enum class Movement {
        NONE,
        UP,
        DOWN
    };

    // Constructor
    InputHandler(uint8_t joyXPin, uint8_t joyYPin, uint8_t buttonPin);
    
    // Initialize pins
    void begin();
    
    // Update and get input states
    void update();
    Movement getVerticalMovement() const { return verticalMovement; }
    bool getToggleRequested() const { return toggleRequested; }
    bool getButtonPressed() const { return buttonPressed; }  // Momentary press
    bool getButtonState() const { return currentButtonState; }  // Current state
    
    // Threshold settings
    void setDeadband(int8_t threshold) { deadbandThreshold = threshold; }
    void setToggleThreshold(int8_t threshold) { toggleThreshold = threshold; }
    void setNavigationThreshold(int8_t threshold) { navigationThreshold = threshold; }

private:
    // Pins
    const uint8_t joystickXPin;
    const uint8_t joystickYPin;
    const uint8_t buttonPin;
    
    // Thresholds (in percentage -100 to 100)
    int8_t deadbandThreshold = 20;     // 20% deadband in the middle
    int8_t toggleThreshold = 50;       // 50% threshold for toggle
    int8_t navigationThreshold = 30;   // 30% threshold for navigation
    
    // State tracking
    int8_t lastXValue = 0;
    int8_t lastYValue = 0;
    bool lastButtonState = false;
    bool currentButtonState = false;    // Added to track current button state
    bool wasInNavigationThreshold = false;
    bool wasInToggleThreshold = false;
    unsigned long lastDebounceTime = 0;
    static const unsigned long DEBOUNCE_DELAY = 50;
    
    // Current states
    Movement verticalMovement = Movement::NONE;
    bool toggleRequested = false;
    bool buttonPressed = false;
    
    // Helper methods
    int8_t mapJoystickValue(int rawValue);
    bool isInDeadband(int8_t value);
    bool exceedsToggleThreshold(int8_t value);
    bool exceedsNavigationThreshold(int8_t value);
}; 