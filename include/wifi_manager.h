#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "credential_manager.h"
#include "time_manager.h"
#include "settings_manager.h"
#include "display_manager.h"
#include "image_manager.h"
#include "secrets.h"    // For credentials only

// Forward declarations
class RotationTester;

class WiFiManager {
public:
    enum OperationMode {
        MODE_SETUP,     // AP Portal for configuration
        MODE_NORMAL     // Main server on WiFi network
    };

private:
    AsyncWebServer* server;
    TimeManager* timeManager;
    SettingsManager* settingsManager;
    DisplayManager* displayManager;
    ImageManager* imageManager;
    RotationTester* rotationTester;
    class SlideshowManager* slideshowManager;
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
    bool switchToPortalMode;

    // New private variables
    bool connectionSuccessDisplayed;
    unsigned long connectionSuccessStartTime;
    
public:
    WiFiManager(AsyncWebServer* webServer, TimeManager* timeManager, SettingsManager* settingsManager, DisplayManager* displayManager, ImageManager* imageManager, class SlideshowManager* slideshowManager = nullptr);
    
    // Only change the hardcoded password to use secrets.h
    void initializeAP(const String& ssid = "Billboard-Portal", const String& password = PORTAL_PASSWORD);
    void setupRoutes();
    void startServer();
    String scanNetworks();
    bool connectToWiFi(const String& ssid, const String& password);
    void handleConnect(AsyncWebServerRequest* request);
    void checkHeapHealth();
    void checkConnectionSuccessDisplay();
    
    // Methods - KEEP AS IS
    bool initializeFromCredentials();
    bool connectToSavedNetwork();
    void switchToNormalMode();
    void switchToSetupMode();
    void checkConnectionStatus();
    void checkGpio0FactoryReset();
    void setupNormalModeRoutes();
    void setupImageRoutes();  // New method for image API endpoints
    void setupRotationTestRoutes();  // New method for rotation test endpoints
    void checkScheduledRestart();
    void checkPortalModeSwitch();
    
    // Getters - KEEP AS IS
    OperationMode getCurrentMode() const { return currentMode; }
    bool isConnectedToWiFi() const;
    String getWiFiIP() const;
    bool isShowingConnectionSuccess() const;
};