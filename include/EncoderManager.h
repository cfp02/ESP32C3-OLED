#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

class EncoderManager {
public:
    EncoderManager();
    
    // Initialization
    bool begin();
    
    // Basic encoder operations
    word getRawAngle();     // Get raw angle (0-4095)
    float getDegrees();     // Get angle in degrees (0-360)
    float getRadians();     // Get angle in radians (0-2π)
    
    // Status checks
    bool isMagnetDetected();    // Check if magnet is detected
    int getMagnitude();         // Get magnetic field strength
    
    // Configuration
    void setDirection(bool clockwise);  // Set counting direction
    
private:
    AMS_5600 ams5600;   // Instance of the AS5600 library
    float convertRawAngleToDegrees(word rawAngle);
}; 