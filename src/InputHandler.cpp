#include "InputHandler.h"

InputHandler::InputHandler(uint8_t joyXPin, uint8_t joyYPin, uint8_t buttonPin)
    : joystickXPin(joyXPin), joystickYPin(joyYPin), buttonPin(buttonPin) {
}

void InputHandler::begin() {
    pinMode(joystickXPin, INPUT);
    pinMode(joystickYPin, INPUT);
    pinMode(buttonPin, INPUT_PULLUP);
}

int8_t InputHandler::mapJoystickValue(int rawValue) {
    return map(rawValue, 0, 4095, -100, 100);
}

bool InputHandler::isInDeadband(int8_t value) {
    return abs(value) < deadbandThreshold;
}

bool InputHandler::exceedsToggleThreshold(int8_t value) {
    return abs(value) > toggleThreshold;
}

bool InputHandler::exceedsNavigationThreshold(int8_t value) {
    return abs(value) > navigationThreshold;
}

void InputHandler::update() {
    // Reset event states
    verticalMovement = Movement::NONE;
    toggleRequested = false;
    buttonPressed = false;
    
    // Read joystick values and map to -100 to 100 range
    int8_t xValue = mapJoystickValue(analogRead(joystickXPin));
    int8_t yValue = mapJoystickValue(analogRead(joystickYPin));
    bool rawButtonState = !digitalRead(buttonPin);  // Active LOW
    
    // Handle button press with debouncing
    if (rawButtonState != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        // If the button state has changed
        if (rawButtonState != currentButtonState) {
            currentButtonState = rawButtonState;
            // If it's a press (not a release), set buttonPressed
            if (currentButtonState) {
                buttonPressed = true;
            }
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
            verticalMovement = Movement::UP;
        } else if (lastYValue < 0) {
            verticalMovement = Movement::DOWN;
        }
    }
    
    // Toggle event detection
    if (inToggleThreshold && !wasInToggleThreshold) {
        // Just entered threshold, do nothing
    } else if (!inToggleThreshold && wasInToggleThreshold) {
        // Left threshold, check if we should toggle
        if (abs(lastXValue) > toggleThreshold) {
            toggleRequested = true;
        }
    }
    
    // Update state tracking
    wasInNavigationThreshold = inNavigationThreshold;
    wasInToggleThreshold = inToggleThreshold;
    lastXValue = xValue;
    lastYValue = yValue;
    lastButtonState = rawButtonState;
} 