#include "BLEHandler.h"
#include <Arduino.h>

// Server callbacks implementation
class BLEHandler::ServerCallbacks : public BLEServerCallbacks {
    BLEHandler* handler;
public:
    ServerCallbacks(BLEHandler* h) : handler(h) {}
    
    void onConnect(BLEServer* pServer) {
        handler->onClientConnect();
    }
    
    void onDisconnect(BLEServer* pServer) {
        handler->onClientDisconnect();
    }
};

// Characteristic callbacks implementation
class BLEHandler::CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.print("Received: ");
            Serial.println(value.c_str());
        }
    }
};

BLEHandler::BLEHandler() : pServer(nullptr), pService(nullptr), pCharacteristic(nullptr), 
                          bleConnected(false), bleAdvertising(false), connectedDeviceRSSI(0),
                          pBLEScan(nullptr), pClient(nullptr), bleScanning(false), 
                          scanStartTime(0), pRemoteService(nullptr), pRemoteCharacteristic(nullptr), 
                          serviceDiscovered(false) {
}

BLEHandler::~BLEHandler() {
    // Clean up discovered devices
    for (auto device : discoveredDevices) {
        delete device;
    }
    discoveredDevices.clear();
}

void BLEHandler::begin() {
    // Initialize BLE
    BLEDevice::init("ESP32C3-Controller");
    
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));
    
    // Create BLE Service
    pService = pServer->createService(SERVICE_UUID);
    
    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());
    
    // Start the service
    pService->start();
    
    Serial.println("BLE Server initialized");
    Serial.printf("Service UUID: %s\n", SERVICE_UUID);
    Serial.printf("Characteristic UUID: %s\n", CHARACTERISTIC_UUID);
    Serial.printf("Device Name: ESP32C3-Controller\n");
    Serial.printf("MAC Address: %s\n", BLEDevice::getAddress().toString().c_str());
}

void BLEHandler::startAdvertising() {
    if (!bleAdvertising) {
        // Create advertising
        BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
        pAdvertising->setMinPreferred(0x12);
        
        // Start advertising
        BLEDevice::startAdvertising();
        bleAdvertising = true;
        
        Serial.println("Started advertising - waiting for car to connect...");
    }
}

void BLEHandler::stopAdvertising() {
    if (bleAdvertising) {
        BLEDevice::stopAdvertising();
        bleAdvertising = false;
        Serial.println("Stopped advertising");
    }
}

bool BLEHandler::isAdvertising() const {
    return bleAdvertising;
}

bool BLEHandler::isConnected() const {
    return bleConnected;
}

void BLEHandler::onClientConnect() {
    bleConnected = true;
    Serial.println("Car connected to controller!");
    
    // Stop advertising since we're now connected
    stopAdvertising();
    
    // Get connected device info
    BLEDevice::getAddress().toString();
    connectedDeviceAddress = BLEDevice::getAddress().toString();
    connectedDeviceName = "ESP32S3-Car"; // We know it's the car
    connectedDeviceRSSI = 0; // RSSI not available for server connections
    
    printConnectedDeviceInfo();
}

void BLEHandler::onClientDisconnect() {
    bleConnected = false;
    Serial.println("Car disconnected from controller!");
    
    // Clear device info
    connectedDeviceAddress.clear();
    connectedDeviceName.clear();
    connectedDeviceRSSI = 0;
    
    // Restart advertising to wait for new connection
    startAdvertising();
}

void BLEHandler::printConnectedDeviceInfo() {
    if (bleConnected) {
        Serial.println("=== Connected Device Details ===");
        Serial.printf("Device Name: %s\n", connectedDeviceName.c_str());
        Serial.printf("Device Address: %s\n", connectedDeviceAddress.c_str());
        Serial.printf("Signal Strength (RSSI): %d dBm\n", connectedDeviceRSSI);
        Serial.println("================================");
    } else {
        Serial.println("No device currently connected");
    }
}

void BLEHandler::update() {
    // For server mode, we don't need much update logic
    // The callbacks handle connection state changes
    
    // Print status every 5 seconds
    static unsigned long lastStatusPrint = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastStatusPrint >= 5000) {
        lastStatusPrint = currentTime;
        if (bleConnected) {
            Serial.println("Status: BLE connected");
            printConnectedDeviceInfo();
        } else if (bleAdvertising) {
            Serial.println("Status: BLE advertising - waiting for connection");
        } else {
            Serial.println("Status: BLE not advertising");
        }
    }
}

void BLEHandler::sendJoystickData(int x, int y) {
    if (!bleConnected || !pCharacteristic) {
        return;
    }
    
    // Create data packet: "X:Y" format
    char dataPacket[20];
    snprintf(dataPacket, sizeof(dataPacket), "%d:%d", x, y);
    
    // Send the data
    pCharacteristic->setValue((uint8_t*)dataPacket, strlen(dataPacket));
    pCharacteristic->notify();
    
    Serial.printf("Sent joystick data: %s\n", dataPacket);
}

// Keep these methods for potential future client functionality
bool BLEHandler::discoverServices() {
    // This method is kept for potential future client mode
    // Currently not used in server mode
    return false;
} 