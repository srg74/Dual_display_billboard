/**
 * @file wifi_manager.h
 * @brief Advanced WiFi management system with dual-mode operation for ESP32 dual display billboard
 * 
 * Provides comprehensive WiFi connectivity management with automatic dual-mode operation,
 * web server integration, OTA firmware updates, and robust connection handling.
 * Essential component for remote configuration and content management.
 * 
 * Key Features:
 * - Dual-mode operation (Station + Access Point simultaneous)
 * - Automatic WiFi credential management with persistent storage
 * - Integrated web server for configuration interface
 * - OTA (Over-The-Air) firmware update capabilities
 * - Portal mode for initial setup and troubleshooting
 * - Time synchronization integration with NTP servers
 * - Real-time status monitoring and connection recovery
 * - RESTful API endpoints for system control
 * 
 * Operation Modes:
 * - Station Mode: Connects to existing WiFi networks
 * - Access Point Mode: Creates "Billboard-Portal" hotspot (192.168.4.1)
 * - Dual Mode: Simultaneous AP+STA operation for maximum accessibility
 * - Portal Mode: Configuration interface accessible via web browser
 * 
 * Web Interface Features:
 * - Image upload and management
 * - Display settings configuration
 * - Clock and timezone settings
 * - System diagnostics and status
 * - OTA firmware updates
 * - Network configuration
 * 
 * Security:
 * - WPA2 protected access point (password: "billboard321")
 * - HTTP basic authentication for OTA interface
 * - Credential encryption in LittleFS storage
 * - CORS headers for API security
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 0.9
 * @since v0.9
 */

#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>          // For OTA firmware updates
#include "credential_manager.h"
#include "time_manager.h"
#include "settings_manager.h"
#include "display_manager.h"
#include "image_manager.h"
#include "secrets.h"    // For credentials only

// Forward declarations
class DisplayManager;
class ImageManager;
class SettingsManager;

/**
 * @brief Advanced WiFi management system with dual-mode operation
 * 
 * Comprehensive WiFi connectivity manager that handles both station and access point
 * modes simultaneously, providing robust network connectivity and web-based
 * configuration interface for the ESP32 dual display billboard system.
 * 
 * The WiFiManager automatically manages connection states, provides fallback
 * connectivity options, and integrates web server functionality for remote
 * system management and content updates.
 * 
 * Key Responsibilities:
 * - Network connection establishment and maintenance
 * - Web server hosting for configuration interface
 * - OTA firmware update processing
 * - API endpoint management for system control
 * - Integration with credential storage and time synchronization
 */
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
    class SlideshowManager* slideshowManager;
    class DCCManager* dccManager;
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
    
    // Crash prevention variables
    int authFailureCount;
    unsigned long lastAuthFailureTime;
    
public:
    WiFiManager(AsyncWebServer* webServer, TimeManager* timeManager, SettingsManager* settingsManager, DisplayManager* displayManager, ImageManager* imageManager, class SlideshowManager* slideshowManager = nullptr, class DCCManager* dccManager = nullptr);
    
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
    void setupOTARoutes();    // NEW: OTA firmware update routes
    void checkScheduledRestart();
    void checkPortalModeSwitch();
    
    // OTA Functions
    bool validateFirmwareFilename(const String& filename);
    bool validateFirmwareBinary(uint8_t* data, size_t length);
    String getFirmwareVersion();
    
    // Getters - KEEP AS IS
    OperationMode getCurrentMode() const { return currentMode; }
    bool isConnectedToWiFi() const;
    String getWiFiIP() const;
    bool isShowingConnectionSuccess() const;
};