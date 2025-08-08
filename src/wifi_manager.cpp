#include "wifi_manager.h"
#include "logger.h"
#include "config.h"
#include "webcontent.h"
#include "display_manager.h"

static const String TAG = "WIFI";

const unsigned long WiFiManager::RETRY_DELAYS[] = {5000, 10000, 30000}; // 5s, 10s, 30s

WiFiManager::WiFiManager(AsyncWebServer* webServer) : server(webServer) {
    LOG_DEBUG(TAG, "WiFiManager constructor called");
    
    // Initialize new Step 2 variables
    currentMode = MODE_SETUP;
    lastConnectionAttempt = 0;
    connectionRetryCount = 0;
    lastGpio0Check = 0;
    gpio0Pressed = false;
    gpio0PressStart = 0;
    restartPending = false;
    restartScheduledTime = 0;
    
    // FIX: Uncomment these lines
    connectionSuccessDisplayed = false;
    connectionSuccessStartTime = 0;
    
    // Initialize GPIO0 pin for factory reset
    pinMode(GPIO0_PIN, INPUT_PULLUP);
}

void WiFiManager::initializeAP(const String& ssid, const String& password) {
    apSSID = ssid;
    apPassword = password;
    
    LOG_INFO(TAG, "=== Starting Access Point ===");
    LOG_INFOF(TAG, "SSID: '%s'", ssid.c_str());
    LOG_INFOF(TAG, "Password: '%s'", password.c_str());
    
    // OPTIMIZATION: Show quick starting indicator (non-blocking)
    extern DisplayManager displayManager;
    displayManager.showAPStarting();  // Fast "Starting AP..." message
    
    // FAST PATH: Do critical WiFi setup without display delays
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    LOG_DEBUG(TAG, "WiFi stopped, configuring AP mode...");
    
    // Configure custom IP for captive portal
    IPAddress local_IP(4, 3, 2, 1);
    IPAddress gateway(4, 3, 2, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    LOG_DEBUGF(TAG, "Setting AP IP to: %s", local_IP.toString().c_str());
    
    // Set AP mode and configure IP (fastest path)
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        LOG_ERROR(TAG, "❌ Failed to configure AP IP");
        displayManager.showQuickStatus("AP Config Failed", TFT_RED);
        return;
    }
    
    LOG_DEBUG(TAG, "AP IP configured, starting access point...");
    
    // Start the Access Point (critical path)
    bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), 1, 0, 4);
    
    if (apStarted) {
        // Quick success feedback
        displayManager.showAPReady();  // Fast "AP Ready!" message
        
        IPAddress IP = WiFi.softAPIP();
        LOG_INFOF(TAG, "✅ Access Point started successfully! SSID: %s, IP: %s", 
                  apSSID.c_str(), IP.toString().c_str());
        
        currentMode = MODE_SETUP;
        
        // AFTER AP IS READY: Show detailed portal info (non-critical timing)
        displayManager.showPortalInfo(
            PORTAL_SSID,           
            "IP: 4.3.2.1",        
            "Ready to connect"     
        );
        
    } else {
        LOG_ERROR(TAG, "❌ Failed to start Access Point!");
        LOG_ERROR(TAG, "Check if another AP is running or SSID is too long");
        
        // Show error status
        displayManager.showQuickStatus("AP Start Failed", TFT_RED);
    }
}

void WiFiManager::setupRoutes() {
    LOG_INFO(TAG, "=== Setting up web server routes ===");
    
    // Test route for connectivity
    server->on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 Test route accessed!");
        request->send(200, "text/plain", "Billboard server is working!\nTime: " + String(millis()));
    });

    // Main portal route
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 Portal page requested");
        
        // Check available memory before serving large content
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 50000) {  // Less than 50KB free
            LOG_WARNF(TAG, "⚠️ Low memory (%d bytes), serving minimal page", freeHeap);
            request->send(200, "text/html", 
                "<html><body style='font-family:Arial;padding:2rem;'>"
                "<h1>Billboard Portal</h1>"
                "<p>Low memory - please restart device</p>"
                "<p><a href='/status'>Check Status</a></p>"
                "</body></html>");
            return;
        }
        
        // Get portal HTML with error checking
        String html = getPortalHTML();
        if (html.length() == 0) {
            LOG_ERROR(TAG, "❌ Portal HTML not available!");
            request->send(500, "text/plain", "Portal HTML generation failed");
            return;
        }
        
        LOG_INFOF(TAG, "✅ Serving portal HTML (%d bytes), Free heap: %d", 
                  html.length(), freeHeap);
        
        // Use chunked response for large content
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
        response->addHeader("Connection", "close");  // Force close to free memory
        request->send(response);
    });
    
    // WiFi scan route
    server->on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 WiFi scan requested");
        String networks = this->scanNetworks();
        request->send(200, "application/json", networks);
    });
    
    // WiFi connect route
    server->on("/connect", HTTP_POST, [this](AsyncWebServerRequest *request){
        this->handleConnect(request);
    });
    
    // Status route
    server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String status = "{";
        status += "\"uptime\":\"" + String(millis() / 1000) + " seconds\",";
        status += "\"memory\":\"" + String(ESP.getFreeHeap()) + " bytes\"";
        status += "}";
        request->send(200, "application/json", status);
    });
    
    // WiFi status route
    server->on("/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request){
        String status = "{";
        status += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
        status += "\"ssid\":\"" + WiFi.SSID() + "\",";
        status += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        status += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        status += "\"ap_clients\":" + String(WiFi.softAPgetStationNum());
        status += "}";
        request->send(200, "application/json", status);
    });
    
    // Captive portal routes for different devices
    server->on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "Android captive portal check");
        request->redirect("/");
    });
    
    server->on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "iOS captive portal check");
        request->redirect("/");
    });
    
    server->on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "Windows captive portal check");
        request->redirect("/");
    });
    
    // Handle any other requests
    server->onNotFound([](AsyncWebServerRequest *request){
        LOG_WARNF(TAG, "⚠️ 404 - Not found: %s", request->url().c_str());
        request->redirect("/");
    });
    
    LOG_INFO(TAG, "✅ Web routes configured successfully");
}

void WiFiManager::startServer() {
    LOG_INFO(TAG, "🚀 Starting web server...");
    server->begin();
    LOG_INFO(TAG, "✅ Web server started and listening on port 80");
}

String WiFiManager::scanNetworks() {
    LOG_INFO(TAG, "Scanning for WiFi networks...");
    
    // Clear any previous scan results first
    WiFi.scanDelete();
    
    int n = WiFi.scanNetworks(false, true); // async=false, show_hidden=true
    
    if (n == WIFI_SCAN_FAILED) {
        LOG_ERROR(TAG, "WiFi scan failed");
        return "[]";
    }
    
    String networks = "[";
    int validNetworkCount = 0;
    
    // Process all found networks, but only include valid ones
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        
        // FILTER: Skip networks with empty or blank SSIDs
        if (ssid.length() == 0) {
            LOG_DEBUGF(TAG, "Skipping network %d with empty SSID", i);
            continue;
        }
        
        // FILTER: Skip whitespace-only SSIDs
        ssid.trim(); // Trim in place (modifies the string)
        if (ssid.length() == 0) {
            LOG_DEBUGF(TAG, "Skipping network %d with whitespace-only SSID", i);
            continue;
        }
        
        // FILTER: Skip very weak signals (optional - less than -90 dBm)
        if (WiFi.RSSI(i) < -90) {
            LOG_DEBUGF(TAG, "Skipping weak network: %s (%d dBm)", ssid.c_str(), WiFi.RSSI(i));
            continue;
        }
        
        // Add comma separator for valid networks (not first one)
        if (validNetworkCount > 0) networks += ",";
        
        // Escape quotes in SSID
        ssid.replace("\"", "\\\"");
        
        // Add valid network to JSON
        networks += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        validNetworkCount++;
        
        // Limit to 10 valid networks to prevent memory issues
        if (validNetworkCount >= 10) {
            LOG_DEBUGF(TAG, "Reached maximum of 10 networks, stopping scan");
            break;
        }
    }
    
    networks += "]";
    
    // Clean up scan results to free memory
    WiFi.scanDelete();
    
    LOG_INFOF(TAG, "Found %d total networks, showing %d valid networks", n, validNetworkCount);
    return networks;
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
    LOG_INFOF(TAG, "Attempting to connect to WiFi: %s", ssid.c_str());
    
    // Start the connection
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout (non-blocking in chunks)
    unsigned long startTime = millis();
    const unsigned long timeout = 15000; // 15 seconds timeout
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        yield(); // Allow other tasks
        delayMicroseconds(500000); // 500ms delay in microseconds (non-blocking)
        
        // Log progress every 2 seconds
        if ((millis() - startTime) % 2000 < 500) {
            LOG_DEBUGF(TAG, "Connection attempt... Status: %d", WiFi.status());
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFOF(TAG, "✅ Connected successfully! IP: %s", WiFi.localIP().toString().c_str());
        LOG_INFOF(TAG, "✅ Signal strength: %d dBm", WiFi.RSSI());
        return true;
    } else {
        LOG_ERRORF(TAG, "❌ Connection failed. Status: %d", WiFi.status());
        WiFi.disconnect(); // Clean up failed connection
        return false;
    }
}

void WiFiManager::handleConnect(AsyncWebServerRequest* request) {
    LOG_INFO(TAG, "WiFi connection request received");
    
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        
        LOG_INFOF(TAG, "Starting connection to: %s", ssid.c_str());
        
        // Attempt connection
        bool connected = connectToWiFi(ssid, password);
        
        if (connected) {
            // Save credentials
            if (CredentialManager::saveCredentials(ssid, password)) {
                LOG_INFO(TAG, "✅ Credentials saved successfully");
            } else {
                LOG_WARN(TAG, "⚠️ Failed to save credentials, but connection succeeded");
            }
            
            // Get the IP address for user instructions
            String deviceIP = WiFi.localIP().toString();
            
            // Send success response
            String response = "{";
            response += "\"status\":\"success\",";
            response += "\"message\":\"Successfully connected to " + ssid + "! Device will restart and be available at " + deviceIP + "\",";
            response += "\"restart\":true,";
            response += "\"ip\":\"" + deviceIP + "\",";
            response += "\"ssid\":\"" + ssid + "\"";
            response += "}";
            
            request->send(200, "application/json", response);
            LOG_INFOF(TAG, "✅ Connection successful - device will be available at %s", deviceIP.c_str());
            
            // NON-BLOCKING RESTART: Schedule restart instead of immediate delay
            restartPending = true;
            restartScheduledTime = millis() + 3000;  // 3 seconds from now
            LOG_INFO(TAG, "🔄 Restart scheduled in 3 seconds...");
            
        } else {
            String response = "{\"status\":\"error\",\"message\":\"Failed to connect to " + ssid + ". Check password and signal strength.\"}";
            request->send(400, "application/json", response);
            LOG_ERROR(TAG, "❌ Connection failed");
        }
    } else {
        LOG_ERROR(TAG, "Missing SSID or password in request");
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID or password\"}");
    }
}

void WiFiManager::checkHeapHealth() {
    size_t freeHeap = ESP.getFreeHeap();
    size_t minFreeHeap = ESP.getMinFreeHeap();
    
    if (freeHeap < 50000) { // Less than 50KB free
        LOG_WARNF(TAG, "⚠️ Low memory warning: %d bytes free", freeHeap);
    }
    
    if (minFreeHeap < 30000) { // Minimum ever was less than 30KB
        LOG_ERRORF(TAG, "❌ Critical memory usage detected: min %d bytes", minFreeHeap);
    }
}

// ========================================
// Step 2: Mode Management & Auto-Connect
// ========================================

bool WiFiManager::initializeFromCredentials() {
    LOG_INFO(TAG, "🔍 Checking for saved WiFi credentials...");
    
    if (!CredentialManager::hasCredentials()) {
        LOG_INFO(TAG, "📄 No saved credentials found - starting setup mode");
        switchToSetupMode();
        return false;
    }
    
    CredentialManager::WiFiCredentials creds = CredentialManager::loadCredentials();
    if (!creds.isValid) {
        LOG_ERROR(TAG, "❌ Invalid credentials found - starting setup mode");
        switchToSetupMode();
        return false;
    }
    
    LOG_INFOF(TAG, "✅ Found credentials for: %s", creds.ssid.c_str());
    
    // Try to connect to saved network
    if (connectToSavedNetwork()) {
        switchToNormalMode();
        return true;
    } else {
        LOG_WARN(TAG, "⚠️ Auto-connect failed - falling back to setup mode");
        switchToSetupMode();
        return false;
    }
}

bool WiFiManager::connectToSavedNetwork() {
    CredentialManager::WiFiCredentials creds = CredentialManager::loadCredentials();
    if (!creds.isValid) {
        return false;
    }
    
    LOG_INFOF(TAG, "🔗 Attempting auto-connect to: %s", creds.ssid.c_str());
    
    // Stop AP mode and switch to STA mode
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    // Attempt connection
    bool connected = connectToWiFi(creds.ssid, creds.password);
    
    if (connected) {
        LOG_INFOF(TAG, "✅ Auto-connected successfully! IP: %s", WiFi.localIP().toString().c_str());
        connectionRetryCount = 0; // Reset retry counter
        
        // NEW: Show connection success message
        extern DisplayManager displayManager;
        displayManager.showConnectionSuccess(WiFi.localIP().toString());
        connectionSuccessDisplayed = true;
        connectionSuccessStartTime = millis();
        
        return true;
    } else {
        LOG_ERROR(TAG, "❌ Auto-connect failed");
        return false;
    }
}

void WiFiManager::switchToNormalMode() {
    LOG_INFO(TAG, "🔄 Switching to NORMAL mode");
    currentMode = MODE_NORMAL;
    
    // Stop the current server
    server->end();
    
    // Setup routes for normal mode
    setupNormalModeRoutes();
    
    // Start server again
    startServer();
    
    LOG_INFOF(TAG, "✅ Normal mode active - server running on WiFi IP: %s", WiFi.localIP().toString().c_str());
}

void WiFiManager::switchToSetupMode() {
    LOG_INFO(TAG, "🔄 Switching to SETUP mode");
    currentMode = MODE_SETUP;
    
    unsigned long startTime = millis();
    LOG_INFOF(TAG, "⏱️ Setup mode start time: %lu ms", startTime);
    
    // Stop current server
    LOG_INFO(TAG, "🛑 Stopping current server...");
    server->end();
    
    // Disconnect from WiFi and start AP
    LOG_INFO(TAG, "📶 Disconnecting from WiFi...");
    WiFi.disconnect(true);
    
    LOG_INFO(TAG, "🏗️ Starting Access Point...");
    initializeAP(PORTAL_SSID, PORTAL_PASSWORD);
    
    // Setup portal routes
    LOG_INFO(TAG, "🛣️ Setting up portal routes...");
    setupRoutes();
    
    // Start server
    LOG_INFO(TAG, "🚀 Starting server...");
    startServer();
    
    unsigned long endTime = millis();
    LOG_INFOF(TAG, "⏱️ Setup mode completed in: %lu ms", endTime - startTime);
    
    LOG_INFO(TAG, "✅ Setup mode active - portal running at http://4.3.2.1");
}

void WiFiManager::setupNormalModeRoutes() {
    LOG_INFO(TAG, "=== Setting up normal mode routes ===");
    
    // Main billboard page
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 Billboard main page requested");
        request->send(200, "text/html", 
            "<html><body style='font-family:Arial;padding:2rem;background:#0d1117;color:#f0f6fc;'>"
            "<h1>🎯 Billboard Controller</h1>"
            "<p><strong>Status:</strong> Connected to WiFi</p>"
            "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>"
            "<p><strong>SSID:</strong> " + WiFi.SSID() + "</p>"
            "<p><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>"
            "<hr><p><em>Billboard main content will be served here</em></p>"
            "</body></html>");
    });
    
    // Status API
    server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String status = "{";
        status += "\"mode\":\"normal\",";
        status += "\"connected\":true,";
        status += "\"ssid\":\"" + WiFi.SSID() + "\",";
        status += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        status += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        status += "\"uptime\":\"" + String(millis() / 1000) + " seconds\",";
        status += "\"memory\":\"" + String(ESP.getFreeHeap()) + " bytes\"";
        status += "}";
        request->send(200, "application/json", status);
    });
    
    // Factory reset endpoint (for debugging)
    server->on("/factory-reset", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_WARN(TAG, "🏭 Factory reset requested via web interface");
        CredentialManager::clearCredentials();
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Credentials cleared. Restarting...\"}");
        restartPending = true;
        restartScheduledTime = millis() + 1000;  // 1 second restart
    });
    
    LOG_INFO(TAG, "✅ Normal mode routes configured");
}

void WiFiManager::checkConnectionStatus() {
    if (currentMode != MODE_NORMAL) return;
    
    unsigned long currentTime = millis();
    
    // Check connection every 30 seconds
    if (currentTime - lastConnectionAttempt < 30000) return;
    
    if (WiFi.status() != WL_CONNECTED) {
        connectionRetryCount++;
        LOG_WARNF(TAG, "⚠️ WiFi connection lost - retry attempt %d/%d", connectionRetryCount, MAX_RETRY_ATTEMPTS);
        
        if (connectionRetryCount <= MAX_RETRY_ATTEMPTS) {
            // Attempt reconnection with increasing delays
            unsigned long delay = RETRY_DELAYS[min(connectionRetryCount - 1, 2)];
            if (currentTime - lastConnectionAttempt >= delay) {
                lastConnectionAttempt = currentTime;
                
                if (connectToSavedNetwork()) {
                    LOG_INFO(TAG, "✅ WiFi reconnected successfully");
                    connectionRetryCount = 0;
                } else {
                    LOG_ERRORF(TAG, "❌ Reconnection attempt %d failed", connectionRetryCount);
                }
            }
        } else {
            LOG_WARN(TAG, "⚠️ Max retry attempts reached - staying in normal mode, background retries continue");
            // Reset counter for background retries every 60 seconds
            if (currentTime - lastConnectionAttempt >= 60000) {
                connectionRetryCount = 0;
                lastConnectionAttempt = currentTime;
            }
        }
    }
}

void WiFiManager::checkGpio0FactoryReset() {
    unsigned long currentTime = millis();
    
    // Check GPIO0 every 100ms
    if (currentTime - lastGpio0Check < 100) return;
    lastGpio0Check = currentTime;
    
    bool gpio0State = digitalRead(GPIO0_PIN) == LOW; // LOW when pressed (pullup)
    
    if (gpio0State && !gpio0Pressed) {
        // Button just pressed
        gpio0Pressed = true;
        gpio0PressStart = currentTime;
        LOG_DEBUG(TAG, "🔘 GPIO0 button pressed");
        
    } else if (!gpio0State && gpio0Pressed) {
        // Button released
        gpio0Pressed = false;
        unsigned long pressDuration = currentTime - gpio0PressStart;
        LOG_DEBUGF(TAG, "🔘 GPIO0 button released after %lu ms", pressDuration);
        
    } else if (gpio0State && gpio0Pressed) {
        // Button held down - check duration
        unsigned long pressDuration = currentTime - gpio0PressStart;
        
        if (pressDuration >= FACTORY_RESET_DURATION) {
            LOG_WARN(TAG, "🏭 FACTORY RESET TRIGGERED! (GPIO0 held for 6+ seconds)");
            
            // Clear credentials and restart in setup mode
            CredentialManager::clearCredentials();
            gpio0Pressed = false; // Prevent multiple triggers
            
            // Restart the system
            LOG_INFO(TAG, "🔄 Restarting system...");
            restartPending = true;
            restartScheduledTime = millis() + 1000;  // 1 second restart
        }
    }
}

bool WiFiManager::isConnectedToWiFi() const {
    return (WiFi.status() == WL_CONNECTED && currentMode == MODE_NORMAL);
}

String WiFiManager::getWiFiIP() const {
    if (currentMode == MODE_NORMAL && WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "N/A";
}

void WiFiManager::checkScheduledRestart() {
    if (restartPending && millis() >= restartScheduledTime) {
        LOG_INFO(TAG, "🔄 Executing scheduled restart...");
        ESP.restart();
    }
}

void WiFiManager::checkConnectionSuccessDisplay() {
    // Only check if we're displaying the connection success message
    if (!connectionSuccessDisplayed) return;
    
    // Check if 5 seconds have passed
    if (millis() - connectionSuccessStartTime >= 5000) {
        LOG_INFO(TAG, "🕐 5 seconds passed, switching to normal mode display");
        
        // Mark as no longer displaying
        connectionSuccessDisplayed = false;
        
        LOG_INFO(TAG, "✅ Switched to normal mode display");
    }
}

// NEW: Getter method for connection success display state
bool WiFiManager::isShowingConnectionSuccess() const {
    return connectionSuccessDisplayed;
}

