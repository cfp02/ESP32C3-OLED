#include <Arduino.h>
#include "OLEDScreen.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BUTTON_PIN GPIO_NUM_0
#define POT_X_PIN GPIO_NUM_2
#define POT_Y_PIN GPIO_NUM_1
#define LED_PIN GPIO_NUM_8

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define JOYSTICK_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define STATUS_CHAR_UUID    "5b818d26-7c11-4f24-b87f-4f8a8cc974eb"

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
const unsigned long PAIRING_WINDOW = 2000; // 2 second pairing window

// Last update time
unsigned long lastUpdate = 0;
unsigned long startTime = 0;

// Buffer for formatted strings
char displayBuffer[20];

// BLE variables
BLEServer* pServer = NULL;
BLECharacteristic* pJoystickCharacteristic = NULL;
BLECharacteristic* pStatusCharacteristic = NULL;
bool deviceConnected = false;
bool pairingMode = true;

// BLE callbacks
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      pStatusCharacteristic->setValue("Connected");
      pStatusCharacteristic->notify();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      pStatusCharacteristic->setValue("Disconnected");
      pStatusCharacteristic->notify();
      BLEDevice::startAdvertising();
    }
};

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(1000);
    Serial.println("Joystick Controller Starting...");
    
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

    // Initialize BLE
    BLEDevice::init("JoystickCtrl");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristics
    pJoystickCharacteristic = pService->createCharacteristic(
        JOYSTICK_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pJoystickCharacteristic->addDescriptor(new BLE2902());

    pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    pStatusCharacteristic->setValue("Pairing...");
    pStatusCharacteristic->notify();

    startTime = millis();
}

void loop() {
    unsigned long currentTime = millis();
    
    // Check if we're still in pairing mode
    if (pairingMode && (currentTime - startTime > PAIRING_WINDOW)) {
        pairingMode = false;
        pStatusCharacteristic->setValue("Ready");
        pStatusCharacteristic->notify();
    }
    
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
        
        // Display connection status
        screen.setCursor(0, 47);
        if (pairingMode) {
            screen.print("Pairing...");
        } else if (deviceConnected) {
            screen.print("Connected");
        } else {
            screen.print("Disconnected");
        }
        
        screen.update();
        
        // Send joystick data if connected
        if (deviceConnected) {
            char joystickData[20];
            snprintf(joystickData, sizeof(joystickData), "%d,%d,%d", potX_pct, potY_pct, buttonState);
            pJoystickCharacteristic->setValue(joystickData);
            pJoystickCharacteristic->notify();
        }
        
        // Update the last update time
        lastUpdate = currentTime;
    }
}
 