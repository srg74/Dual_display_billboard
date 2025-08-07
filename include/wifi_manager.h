#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "credential_manager.h"
#include "secrets.h"    // For credentials only

class WiFiManager {
public:
    enum OperationMode {
        MODE_SETUP,     // AP Portal for configuration
        MODE_NORMAL     // Main server on WiFi network
    };

private:
    AsyncWebServer* server;
    String apSSID;
    String apPassword;
    OperationMode currentMode;
    unsigned long lastConnectionAttempt;
    int connectionRetryCount;
    unsigned long lastGpio0Check;
    bool gpio0Pressed;
    unsigned long gpio0PressStart;
    
    // Constants - KEEP AS YOU HAD THEM (working fine)
    static const int MAX_RETRY_ATTEMPTS = 3;
    static const unsigned long RETRY_DELAYS[];
    static const unsigned long FACTORY_RESET_DURATION = 6000; // 6 seconds
    static const int GPIO0_PIN = 0;
    
    bool restartPending;
    unsigned long restartScheduledTime;
    
public:
    WiFiManager(AsyncWebServer* webServer);
    
    // Only change the hardcoded password to use secrets.h
    void initializeAP(const String& ssid = "Billboard-Portal", const String& password = PORTAL_PASSWORD);
    void setupRoutes();
    void startServer();
    String scanNetworks();
    bool connectToWiFi(const String& ssid, const String& password);
    void handleConnect(AsyncWebServerRequest* request);
    void checkHeapHealth();
    
    // Methods - KEEP AS IS
    bool initializeFromCredentials();
    bool connectToSavedNetwork();
    void switchToNormalMode();
    void switchToSetupMode();
    void checkConnectionStatus();
    void checkGpio0FactoryReset();
    void setupNormalModeRoutes();
    void checkScheduledRestart();
    
    // Getters - KEEP AS IS
    OperationMode getCurrentMode() const { return currentMode; }
    bool isConnectedToWiFi() const;
    String getWiFiIP() const;
};