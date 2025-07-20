#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLECharacteristic.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <vector>

// Target car MAC address for short-circuiting
#define TARGET_CAR_MAC "34:85:18:91:cc:d5"

// BLE service and characteristic UUIDs from the car
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"

class BLEHandler {
public:
    BLEHandler();
    ~BLEHandler();
    
    void begin();
    void startAdvertising();
    void stopAdvertising();
    bool isAdvertising() const;
    bool isConnected() const;
    void update();
    void printConnectedDeviceInfo();
    void sendJoystickData(int x, int y);
    
    // Callback for when client connects/disconnects
    void onClientConnect();
    void onClientDisconnect();
    
private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pCharacteristic;
    bool bleConnected;
    bool bleAdvertising;
    
    // Store connected device info
    std::string connectedDeviceAddress;
    std::string connectedDeviceName;
    int connectedDeviceRSSI;
    
    // Server callbacks
    class ServerCallbacks;
    class CharacteristicCallbacks;
    
    // Keep client functionality for potential future use
    BLEScan* pBLEScan;
    BLEClient* pClient;
    bool bleScanning;
    unsigned long scanStartTime;
    std::vector<BLEAdvertisedDevice*> discoveredDevices;
    
    // BLE service and characteristic handles (for client mode)
    BLERemoteService* pRemoteService;
    BLERemoteCharacteristic* pRemoteCharacteristic;
    bool serviceDiscovered;
    
    bool discoverServices();
}; 