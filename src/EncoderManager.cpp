#include "EncoderManager.h"

EncoderManager::EncoderManager() {
}

bool EncoderManager::begin() {
    // Initialize I2C with explicit pin assignment for ESP32-C3
    // Using the same pins as the OLED display since they share the I2C bus
    Wire.begin(GPIO_NUM_5, GPIO_NUM_6);  // SDA = GPIO5, SCL = GPIO6
    Wire.setClock(100000);  // Set to 100kHz for better stability
    
    // Check if we can communicate with the sensor
    return ams5600.detectMagnet() == 1;
}

word EncoderManager::getRawAngle() {
    return ams5600.getRawAngle();
}

float EncoderManager::getDegrees() {
    return convertRawAngleToDegrees(ams5600.getRawAngle());
}

float EncoderManager::getRadians() {
    return (convertRawAngleToDegrees(ams5600.getRawAngle()) * PI) / 180.0;
}

bool EncoderManager::isMagnetDetected() {
    return ams5600.detectMagnet() == 1;
}

int EncoderManager::getMagnitude() {
    return ams5600.getMagnitude();
}

void EncoderManager::setDirection(bool clockwise) {
    // Note: Direction is set by hardware pin, not through I2C
    // This function is included for API completeness
    (void)clockwise;  // Prevent unused parameter warning
}

float EncoderManager::convertRawAngleToDegrees(word rawAngle) {
    // Raw data reports 0 - 4095 segments, which is 0.087890625 of a degree
    return rawAngle * 0.087890625;
} 