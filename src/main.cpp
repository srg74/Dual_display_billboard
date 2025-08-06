#include <Arduino.h>
#include "logger.h"
#include "credential_manager.h"
#include "wifi_manager.h"

AsyncWebServer server(80);
WiFiManager wifiManager(&server);

// Non-blocking timing variables
unsigned long lastHeapCheck = 0;
unsigned long lastConnectionCheck = 0;
unsigned long lastGpio0Check = 0;
const unsigned long HEAP_CHECK_INTERVAL = 5000; // 5 seconds
const unsigned long CONNECTION_CHECK_INTERVAL = 1000; // 1 second
const unsigned long GPIO0_CHECK_INTERVAL = 100; // 100ms

void setup() {
    Logger::init();
    LOG_SYSTEM_INFO();
    
    // Initialize credential storage
    if (!CredentialManager::begin()) {
        LOG_ERROR("MAIN", "❌ Failed to initialize credential storage!");
        return;
    }
    
    // Try to initialize from saved credentials
    LOG_INFO("MAIN", "🚀 Starting Billboard Controller...");
    
    if (wifiManager.initializeFromCredentials()) {
        LOG_INFO("MAIN", "✅ Started in NORMAL mode with saved credentials");
    } else {
        LOG_INFO("MAIN", "✅ Started in SETUP mode - portal available at http://4.3.2.1");
    }
    
    LOG_INFO("MAIN", "✅ Setup completed");
    
    // Initialize timing
    lastHeapCheck = millis();
    lastConnectionCheck = millis();
    lastGpio0Check = millis();
}

void loop() {
    unsigned long currentTime = millis();
    
    // Check for scheduled restart first
    wifiManager.checkScheduledRestart();
    
    // Non-blocking heap health check
    if (currentTime - lastHeapCheck >= HEAP_CHECK_INTERVAL) {
        wifiManager.checkHeapHealth();
        lastHeapCheck = currentTime;
    }
    
    // Non-blocking connection monitoring
    if (currentTime - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
        wifiManager.checkConnectionStatus();
        lastConnectionCheck = currentTime;
    }
    
    // Non-blocking GPIO0 factory reset check
    if (currentTime - lastGpio0Check >= GPIO0_CHECK_INTERVAL) {
        wifiManager.checkGpio0FactoryReset();
        lastGpio0Check = currentTime;
    }
    
    // Allow other tasks to run
    yield();
}