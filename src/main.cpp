#include <Arduino.h>
#include "wifi_manager.h"
#include "logger.h"
#include "webcontent.h"

AsyncWebServer server(80);
WiFiManager wifiManager(&server);

static const String TAG = "MAIN";
static unsigned long apStartTime = 0;
static bool apStatusLogged = false;
static int lastClientCount = 0; // Track client changes only

void setup() {
    // Initialize logger first
    Logger::init(115200);
    Logger::setLevel(Logger::DEBUG);
    
    LOG_INFO(TAG, "🚀 Billboard System Starting...");
    LOG_SYSTEM_INFO();
    LOG_MEMORY_INFO();
    
    // Test web content availability (without heavy operations)
    LOG_INFOF(TAG, "Available web assets: %d", getAssetCount());
    
    // Initialize WiFi Manager
    LOG_INFO(TAG, "Initializing WiFi Manager...");
    wifiManager.initializeAP("Billboard-Portal", "12345678");
    wifiManager.setupRoutes();
    
    // Start web server
    server.begin();
    LOG_INFO(TAG, "✓ Web server started successfully");
    LOG_INFO(TAG, "📱 Connect to 'Billboard-Portal' with password: 12345678");
    LOG_INFO(TAG, "🌐 Then go to http://4.3.2.1");
    LOG_INFO(TAG, "🔧 System ready!");
    
    apStartTime = millis();
}

void loop() {
    static unsigned long lastMemCheck = 0;
    static unsigned long lastWiFiCheck = 0;
    
    // Log AP status after 2 seconds (only once)
    if (!apStatusLogged && millis() - apStartTime > 2000) {
        apStatusLogged = true;
        LOG_INFOF(TAG, "AP SSID: %s", WiFi.softAPSSID().c_str());
        LOG_INFOF(TAG, "AP IP: %s", WiFi.softAPIP().toString().c_str());
        LOG_INFOF(TAG, "AP MAC: %s", WiFi.softAPmacAddress().c_str());
        LOG_INFOF(TAG, "WiFi Mode: %d (should be 2 for AP)", WiFi.getMode());
    }
    
    // Check client count changes (not every 5 seconds)
    int currentClients = WiFi.softAPgetStationNum();
    if (currentClients != lastClientCount) {
        if (currentClients > lastClientCount) {
            LOG_INFOF(TAG, "🎉 Client connected! Total clients: %d", currentClients);
        } else if (currentClients < lastClientCount) {
            LOG_INFOF(TAG, "👋 Client disconnected. Total clients: %d", currentClients);
        }
        lastClientCount = currentClients;
    }
    
    // Log memory info every 60 seconds (reduce frequency)
    if (millis() - lastMemCheck > 60000) {
        lastMemCheck = millis();
        LOG_MEMORY_INFO();
        wifiManager.checkHeapHealth(); // Add heap monitoring
    }
    
    // Log WiFi status every 2 minutes (reduce frequency)
    if (millis() - lastWiFiCheck > 120000) {
        lastWiFiCheck = millis();
        LOG_WIFI_STATUS();
    }
    
    // Allow other tasks and check for heap corruption
    yield();
    
    // Add a small delay to prevent heap corruption
    delayMicroseconds(100);
}