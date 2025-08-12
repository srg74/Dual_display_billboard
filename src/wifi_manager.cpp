#include "wifi_manager.h"
#include "dcc_manager.h"
#include "logger.h"
#include "config.h"
#include "webcontent.h"
#include "display_manager.h"
#include "slideshow_manager.h"

static const String TAG = "WIFI";

const unsigned long WiFiManager::RETRY_DELAYS[] = {5000, 10000, 30000}; // 5s, 10s, 30s

WiFiManager::WiFiManager(AsyncWebServer* webServer, TimeManager* timeManager, SettingsManager* settingsManager, DisplayManager* displayManager, ImageManager* imageManager, SlideshowManager* slideshowManager, class DCCManager* dccManager) 
    : server(webServer), timeManager(timeManager), settingsManager(settingsManager), displayManager(displayManager), imageManager(imageManager), slideshowManager(slideshowManager), dccManager(dccManager) {
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
    switchToPortalMode = false;
    
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
    bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), 11, 0, 4);
    
    if (apStarted) {
        // Quick success feedback
        displayManager.showAPReady();  // Fast "AP Ready!" message
        
        IPAddress IP = WiFi.softAPIP();
        LOG_INFOF(TAG, "✅ Access Point started successfully! SSID: %s, IP: %s", 
                  apSSID.c_str(), IP.toString().c_str());
        
        currentMode = MODE_SETUP;
        
        // AFTER AP IS READY: Show splash screen (5s) then portal info
        displayManager.showPortalSequence(
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
    
    // Add global request filters to reduce AsyncTCP warnings
    server->addRewrite(new AsyncWebRewrite("/", "/"));
    
    // Test route for connectivity
    server->on("/test", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 Test route accessed!");
        String response = "Billboard server is working!\n";
        response += "Time: " + String(millis()) + "\n";
        response += "Mode: " + String(currentMode == MODE_SETUP ? "Setup" : "Normal") + "\n";
        response += "Free Memory: " + String(ESP.getFreeHeap()) + " bytes\n";
        response += "\nRotation Tester: /debug/rotation-test\n";
        request->send(200, "text/plain", response);
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
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", status);
        response->addHeader("Connection", "close");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
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
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", status);
        response->addHeader("Connection", "close");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
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
    
    // Simple rotation test route that works directly
    server->on("/debug/rotation-test", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🔄 Simple rotation test requested");
        
        if (request->hasParam("rotation")) {
            String rotationStr = request->getParam("rotation")->value();
            int rotation = rotationStr.toInt();
            LOG_INFOF(TAG, "🔄 Testing rotation %d", rotation);
            
            if (rotation >= 0 && rotation <= 3 && displayManager) {
                // Simple test: set rotation and clear screen with colored label
                displayManager->selectDisplay(1);
                TFT_eSPI* tft = displayManager->getTFT(1);
                if (tft) {
                    tft->setRotation(rotation);
                    tft->fillScreen(TFT_BLACK);
                    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
                    tft->setTextSize(3);
                    String label = "ROT " + String(rotation);
                    tft->drawString(label, 10, 40, 2);
                    displayManager->deselectAll();
                    
                    request->send(200, "text/plain", "Rotation " + String(rotation) + " applied - check display");
                } else {
                    request->send(500, "text/plain", "Display not available");
                }
            } else {
                request->send(400, "text/plain", "Invalid rotation (0-3) or display not available");
            }
        } else {
            // Show the web interface
            String html = "<html><head><title>Rotation Test</title></head><body>";
            html += "<h1>Rotation Tester</h1>";
            html += "<p>Click buttons to test rotations:</p>";
            html += "<button onclick=\"test(0)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 0</button>";
            html += "<button onclick=\"test(1)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 1</button>";
            html += "<button onclick=\"test(2)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 2</button>";
            html += "<button onclick=\"test(3)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 3</button>";
            html += "<script>function test(r){fetch('/debug/rotation-test?rotation='+r).then(r=>r.text()).then(t=>alert(t));}</script>";
            html += "</body></html>";
            request->send(200, "text/html", html);
        }
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
    
    // MULTIPLATFORM FIX: For ESP32-S3 in AP mode, temporarily switch to AP+STA for scanning
    wifi_mode_t currentMode = WiFi.getMode();
    bool modeChanged = false;
    
    if (currentMode == WIFI_AP) {
        LOG_DEBUG(TAG, "Switching to AP+STA mode for scanning");
        WiFi.mode(WIFI_AP_STA);
        modeChanged = true;
        delay(100); // Small delay for mode change
    }
    
    // PERFORMANCE FIX: Use synchronous scan for better ESP32-S3 compatibility
    int n = WiFi.scanNetworks(false, true); // async=false, show_hidden=true
    
    // Check if scan failed
    if (n == WIFI_SCAN_FAILED) {
        LOG_ERROR(TAG, "WiFi scan failed to start");
        // Restore original mode if changed
        if (modeChanged) {
            WiFi.mode(currentMode);
        }
        return "[]";
    }
    
    // Restore original WiFi mode if we changed it
    if (modeChanged) {
        LOG_DEBUG(TAG, "Restoring AP mode after scan");
        WiFi.mode(currentMode);
        delay(50); // Small delay for mode change
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
    
    // Main billboard page - FIXED: Use getIndexHTML() instead of hardcoded HTML
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 Billboard main page requested");
        
        String html = getIndexHTML();
        
        if (html.length() == 0) {
            request->send(500, "text/plain", "Index HTML not available");
            return;
        }
        
        // Template replacements
        html.replace("{{WIFI_SSID}}", WiFi.SSID());
        html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
        html.replace("{{TIMEZONE_OPTIONS}}", timeManager ? timeManager->getTimezoneOptions() : "<option value=\"UTC\">UTC</option>");
        html.replace("{{CLOCK_LABEL}}", timeManager ? timeManager->getClockLabel() : "Clock");
        html.replace("{{IMAGE_INTERVAL}}", String(settingsManager->getImageInterval()));
        html.replace("{{GALLERY_IMAGES}}", "No images");
        
        request->send(200, "text/html", html);
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
    
    // Settings page
    server->on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "⚙️ Settings page requested");
        
        String html = getSettingsHTML();
        
        if (html.length() == 0) {
            request->send(500, "text/plain", "Settings HTML not available");
            return;
        }
        
        // Template replacements for settings page
        html.replace("{{WIFI_SSID}}", WiFi.SSID());
        html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
        html.replace("{{WIFI_RSSI}}", String(WiFi.RSSI()));
        html.replace("{{UPTIME}}", String(millis() / 1000));
        html.replace("{{FREE_MEMORY}}", String(ESP.getFreeHeap()));
        html.replace("{{TIMEZONE_OPTIONS}}", timeManager ? timeManager->getTimezoneOptions() : "<option value=\"UTC\">UTC</option>");
        html.replace("{{CURRENT_NTP_SERVER}}", timeManager ? timeManager->getNTPServer1() : "pool.ntp.org");
        
        request->send(200, "text/html", html);
    });
    
    // Time API
    server->on("/time", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (timeManager && timeManager->isTimeValid()) {
            request->send(200, "text/plain", timeManager->getCurrentTime());
        } else {
            request->send(200, "text/plain", "--:--");
        }
    });
    
    // Timezone setting
    server->on("/timezone", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("timezone", true)) {
            String timezone = request->getParam("timezone", true)->value();
            LOG_INFOF(TAG, "📅 Timezone set to: %s", timezone.c_str());
            if (timeManager) {
                timeManager->setTimezone(timezone);
            }
        }
        request->send(200, "text/plain", "OK");
    });
    
    // Clock label setting
    server->on("/clock-label", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("label", true)) {
            String label = request->getParam("label", true)->value();
            LOG_INFOF(TAG, "🏷️ Clock label set to: %s", label.c_str());
            if (timeManager) {
                timeManager->setClockLabel(label);
            }
        }
        request->send(200, "text/plain", "OK");
    });
    
    // Image interval setting
    server->on("/image-interval", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("interval", true)) {
            String interval = request->getParam("interval", true)->value();
            int intervalValue = interval.toInt();
            LOG_INFOF(TAG, "⏱️ Image interval set to: %d seconds", intervalValue);
            
            // Send response immediately to prevent timeout
            request->send(200, "text/plain", "OK");
            
            // Save to persistent settings after response sent
            settingsManager->setImageInterval(intervalValue);
            LOG_DEBUG(TAG, "Image interval saved to persistent storage");
        } else {
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // Second display toggle
    server->on("/second-display", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "📺 Second display endpoint called");
        
        if (request->hasParam("second_display", true)) {
            String enabled = request->getParam("second_display", true)->value();
            bool isEnabled = (enabled == "true");
            LOG_INFOF(TAG, "📺 Second display request: param='%s', parsed=%s", enabled.c_str(), isEnabled ? "true" : "false");
            
            // Send response immediately to prevent timeout
            request->send(200, "text/plain", "OK");
            
            // Save to persistent settings after response sent
            settingsManager->setSecondDisplayEnabled(isEnabled);
            LOG_INFOF(TAG, "📺 Second display setting saved, current value: %s", settingsManager->isSecondDisplayEnabled() ? "true" : "false");
            
            // Apply current brightness to appropriate display(s)
            if (displayManager) {
                uint8_t currentBrightness = settingsManager->getBrightness();
                if (isEnabled) {
                    // Second display enabled - apply brightness to both displays
                    displayManager->setBrightness(currentBrightness, 0); // 0 = both displays
                    LOG_DEBUG(TAG, "Applied current brightness to both displays");
                } else {
                    // Second display disabled - only first display gets brightness, turn off second
                    displayManager->setBrightness(currentBrightness, 1); // 1 = first display only
                    displayManager->setBrightness(0, 2); // Turn off second display
                    LOG_DEBUG(TAG, "Applied brightness to first display only, turned off second");
                }
            }
        } else {
            LOG_WARN(TAG, "⚠️ Missing second_display parameter");
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // DCC interface toggle
    server->on("/dcc", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "🚂 DCC endpoint called");
        
        if (request->hasParam("dcc", true)) {
            String enabled = request->getParam("dcc", true)->value();
            bool isEnabled = (enabled == "true");
            LOG_INFOF(TAG, "🚂 DCC interface request: param='%s', parsed=%s", enabled.c_str(), isEnabled ? "true" : "false");
            
            // Send response immediately to prevent timeout
            request->send(200, "text/plain", "OK");
            
            // Update DCC manager if available
            if (dccManager) {
                dccManager->setEnabled(isEnabled);
            } else {
                // Fallback to settings manager only
                settingsManager->setDCCEnabled(isEnabled);
            }
            LOG_INFOF(TAG, "🚂 DCC setting saved, current value: %s", settingsManager->isDCCEnabled() ? "true" : "false");
        } else {
            LOG_WARN(TAG, "⚠️ Missing dcc parameter");
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // DCC address setting
    server->on("/dccaddress", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "🚂 DCC address endpoint called");
        
        if (request->hasParam("address", true)) {
            String addressStr = request->getParam("address", true)->value();
            int address = addressStr.toInt();
            
            if (address >= 1 && address <= 2048) {
                LOG_INFOF(TAG, "🚂 DCC address request: %d", address);
                
                // Send response immediately
                request->send(200, "text/plain", "OK");
                
                // Update DCC manager if available
                if (dccManager) {
                    dccManager->setAddress(address);
                } else {
                    // Fallback to settings manager only
                    settingsManager->setDCCAddress(address);
                }
                LOG_INFOF(TAG, "🚂 DCC address saved: %d", address);
            } else {
                LOG_WARNF(TAG, "⚠️ Invalid DCC address: %d", address);
                request->send(400, "text/plain", "Invalid address (must be 1-2048)");
            }
        } else {
            LOG_WARN(TAG, "⚠️ Missing address parameter");
            request->send(400, "text/plain", "Missing address parameter");
        }
    });
    
    // DCC pin setting
    server->on("/dccpin", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "🚂 DCC pin endpoint called");
        
        if (request->hasParam("pin", true)) {
            String pinStr = request->getParam("pin", true)->value();
            int pin = pinStr.toInt();
            
            if (pin >= 0 && pin <= 39) {
                LOG_INFOF(TAG, "🚂 DCC pin request: %d", pin);
                
                // Send response immediately
                request->send(200, "text/plain", "OK");
                
                // Update DCC manager if available
                if (dccManager) {
                    dccManager->setPin(pin);
                } else {
                    // Fallback to settings manager only
                    settingsManager->setDCCPin(pin);
                }
                LOG_INFOF(TAG, "🚂 DCC pin saved: %d", pin);
            } else {
                LOG_WARNF(TAG, "⚠️ Invalid DCC pin: %d", pin);
                request->send(400, "text/plain", "Invalid pin (must be 0-39)");
            }
        } else {
            LOG_WARN(TAG, "⚠️ Missing pin parameter");
            request->send(400, "text/plain", "Missing pin parameter");
        }
    });
    
    // DCC address setting
    server->on("/dccaddress", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "🚂 DCC address endpoint called");
        
        if (request->hasParam("address", true)) {
            String addressStr = request->getParam("address", true)->value();
            int address = addressStr.toInt();
            
            // Validate DCC address range (1-2048 for accessories)
            if (address >= 1 && address <= 2048) {
                settingsManager->setDCCAddress(address);
                LOG_INFOF(TAG, "🚂 DCC address set to: %d", address);
                request->send(200, "text/plain", "DCC address updated");
            } else {
                LOG_WARNF(TAG, "⚠️ Invalid DCC address: %d", address);
                request->send(400, "text/plain", "Invalid DCC address (1-2048)");
            }
        } else {
            LOG_WARN(TAG, "⚠️ Missing address parameter");
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // DCC pin setting
    server->on("/dccpin", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "🚂 DCC pin endpoint called");
        
        if (request->hasParam("pin", true)) {
            String pinStr = request->getParam("pin", true)->value();
            int pin = pinStr.toInt();
            
            // Validate GPIO pin range for ESP32
            if (pin >= 0 && pin <= 39) {
                settingsManager->setDCCPin(pin);
                LOG_INFOF(TAG, "🚂 DCC GPIO pin set to: %d", pin);
                request->send(200, "text/plain", "DCC pin updated");
            } else {
                LOG_WARNF(TAG, "⚠️ Invalid GPIO pin: %d", pin);
                request->send(400, "text/plain", "Invalid GPIO pin (0-39)");
            }
        } else {
            LOG_WARN(TAG, "⚠️ Missing pin parameter");
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // Image enable toggle
    server->on("/image-enable", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("image_enable", true)) {
            String enabled = request->getParam("image_enable", true)->value();
            bool isEnabled = (enabled == "true");
            LOG_INFOF(TAG, "🖼️ Image display: %s", isEnabled ? "enabled" : "disabled");
            
            // Send response immediately to prevent timeout
            request->send(200, "text/plain", "OK");
            
            // Save to persistent settings after response sent
            settingsManager->setImageEnabled(isEnabled);
            LOG_DEBUG(TAG, "Image display setting saved to persistent storage");
        } else {
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // Brightness control
    server->on("/brightness", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("brightness", true)) {
            String brightness = request->getParam("brightness", true)->value();
            int brightnessValue = brightness.toInt();
            LOG_INFOF(TAG, "🔆 Brightness set to: %d", brightnessValue);
            
            // Send response immediately to prevent timeout
            request->send(200, "text/plain", "OK");
            
            // Save to persistent settings after response sent
            settingsManager->setBrightness(brightnessValue);
            LOG_DEBUG(TAG, "Brightness setting saved to persistent storage");
            
            // Apply brightness to display(s) based on second display setting
            if (displayManager) {
                bool secondDisplayEnabled = settingsManager->isSecondDisplayEnabled();
                if (secondDisplayEnabled) {
                    // Both displays enabled - set brightness for both
                    displayManager->setBrightness(brightnessValue, 0); // 0 = both displays
                    LOG_DEBUG(TAG, "Applied brightness to both displays");
                } else {
                    // Only main display enabled - Display 1 (BLUE) should stay on, Display 2 (YELLOW) should turn off
                    displayManager->setBrightness(brightnessValue, 1); // Keep Display 1 (BLUE) on with brightness
                    displayManager->setBrightness(0, 2); // Turn off Display 2 (YELLOW)
                    LOG_DEBUG(TAG, "Applied brightness to main display only, turned off second display");
                }
            }
        } else {
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // Clock display toggle
    server->on("/clock", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "🕒 Clock endpoint called");
        
        if (request->hasParam("clock", true)) {
            String enabled = request->getParam("clock", true)->value();
            bool isEnabled = (enabled == "true");
            LOG_INFOF(TAG, "🕒 Clock display request: param='%s', parsed=%s", enabled.c_str(), isEnabled ? "true" : "false");
            
            // Send response immediately to prevent timeout
            request->send(200, "text/plain", "OK");
            
            // Save to persistent settings after response sent
            settingsManager->setClockEnabled(isEnabled);
            LOG_INFOF(TAG, "🕒 Clock setting saved, current value: %s", settingsManager->isClockEnabled() ? "true" : "false");
        } else {
            LOG_WARN(TAG, "⚠️ Missing clock parameter");
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // Clock face selection endpoint
    server->on("/clockface", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "🎨 Clock face endpoint called");
        
        if (request->hasParam("face", true)) {
            String faceStr = request->getParam("face", true)->value();
            int faceType = faceStr.toInt();
            LOG_INFOF(TAG, "🎨 Clock face request: param='%s', parsed=%d", faceStr.c_str(), faceType);
            
            // Validate face type
            if (faceType >= 0 && faceType < 4) {
                // Send response immediately to prevent timeout
                request->send(200, "text/plain", "OK");
                
                // Save to persistent settings after response sent
                settingsManager->setClockFace(static_cast<ClockFaceType>(faceType));
                LOG_INFOF(TAG, "🎨 Clock face setting saved, current value: %d", static_cast<int>(settingsManager->getClockFace()));
            } else {
                LOG_WARN(TAG, "⚠️ Invalid clock face type");
                request->send(400, "text/plain", "Invalid face type");
            }
        } else {
            LOG_WARN(TAG, "⚠️ Missing face parameter");
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // File upload endpoint - REMOVED: Conflicted with image upload endpoint below
    
    // API endpoint for current settings
    server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        String response = "{";
        response += "\"secondDisplay\":" + String(settingsManager->isSecondDisplayEnabled() ? "true" : "false") + ",";
        response += "\"dcc\":" + String(settingsManager->isDCCEnabled() ? "true" : "false") + ",";
        response += "\"dccAddress\":" + String(settingsManager->getDCCAddress()) + ",";
        response += "\"dccPin\":" + String(settingsManager->getDCCPin()) + ",";
        response += "\"clock\":" + String(settingsManager->isClockEnabled() ? "true" : "false") + ",";
        response += "\"clockFace\":" + String(static_cast<int>(settingsManager->getClockFace())) + ",";
        response += "\"brightness\":" + String(settingsManager->getBrightness()) + ",";
        response += "\"imageInterval\":" + String(settingsManager->getImageInterval()) + ",";
        response += "\"imageEnabled\":" + String(settingsManager->isImageEnabled() ? "true" : "false");
        response += "}";
        AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
        apiResponse->addHeader("Connection", "close");
        apiResponse->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(apiResponse);
    });

    // API endpoints for settings page
    server->on("/api/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = "{";
        response += "\"ssid\":\"" + WiFi.SSID() + "\",";
        response += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        response += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        response += "\"status\":\"" + String(WiFi.status() == WL_CONNECTED ? "connected" : "disconnected") + "\"";
        response += "}";
        AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
        apiResponse->addHeader("Connection", "close");
        apiResponse->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(apiResponse);
    });
    
    server->on("/api/system-info", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = "{";
        response += "\"uptime\":" + String(millis() / 1000) + ",";
        response += "\"freeMemory\":" + String(ESP.getFreeHeap());
        response += "}";
        AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
        apiResponse->addHeader("Connection", "close");
        apiResponse->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(apiResponse);
    });
    
    server->on("/api/portal-mode", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 Portal mode activation requested via settings");
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Switching to portal mode...\"}");
        // Set flag to switch to portal mode on next loop
        switchToPortalMode = true;
    });
    
    server->on("/api/ntp-settings", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("server", true)) {
            String ntpServer = request->getParam("server", true)->value();
            LOG_INFOF(TAG, "🕐 NTP server change requested: %s", ntpServer.c_str());
            
            if (ntpServer.length() == 0) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"NTP server cannot be empty\"}");
                return;
            }
            
            // Update NTP server using TimeManager
            if (timeManager) {
                timeManager->setNTPServer(ntpServer);
                LOG_INFOF(TAG, "✅ NTP server updated to: %s", ntpServer.c_str());
                request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"NTP server updated successfully\"}");
            } else {
                LOG_ERROR(TAG, "❌ TimeManager not available");
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"TimeManager not available\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing server parameter\"}");
        }
    });
    
    // Factory reset endpoint (for debugging)
    server->on("/factory-reset", HTTP_POST, [this](AsyncWebServerRequest *request){
        LOG_WARN(TAG, "🏭 Factory reset requested via web interface");
        CredentialManager::clearCredentials();
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Credentials cleared. Restarting...\"}");
        restartPending = true;
        restartScheduledTime = millis() + 1000;  // 1 second restart
    });
    
    // DEBUG: Test endpoints for display debugging
    server->on("/debug/display1", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🔬 Debug: Testing Display 1 brightness");
        if (displayManager) {
            displayManager->setBrightness(255, 1); // Max brightness on Display 1 only
            request->send(200, "text/plain", "Display 1 set to max brightness");
        } else {
            request->send(500, "text/plain", "DisplayManager not available");
        }
    });
    
    server->on("/debug/display2", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🔬 Debug: Testing Display 2 brightness");
        if (displayManager) {
            displayManager->setBrightness(255, 2); // Max brightness on Display 2 only
            request->send(200, "text/plain", "Display 2 set to max brightness");
        } else {
            request->send(500, "text/plain", "DisplayManager not available");
        }
    });
    
    server->on("/debug/display1-off", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🔬 Debug: Turning off Display 1");
        if (displayManager) {
            displayManager->setBrightness(0, 1); // Turn off Display 1
            request->send(200, "text/plain", "Display 1 turned off");
        } else {
            request->send(500, "text/plain", "DisplayManager not available");
        }
    });
    
    server->on("/debug/display2-off", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🔬 Debug: Turning off Display 2");
        if (displayManager) {
            displayManager->setBrightness(0, 2); // Turn off Display 2
            request->send(200, "text/plain", "Display 2 turned off");
        } else {
            request->send(500, "text/plain", "DisplayManager not available");
        }
    });
    
    server->on("/debug/both-on", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🔬 Debug: Turning on both displays");
        if (displayManager) {
            displayManager->setBrightness(255, 0); // Turn on both displays
            request->send(200, "text/plain", "Both displays turned on");
        } else {
            request->send(500, "text/plain", "DisplayManager not available");
        }
    });
    
    // Setup image management routes
    setupImageRoutes();
    
    // Simple inline rotation test for normal mode too
    server->on("/debug/rotation-test", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🔄 Rotation test requested (normal mode)");
        
        if (request->hasParam("rotation")) {
            String rotationStr = request->getParam("rotation")->value();
            int rotation = rotationStr.toInt();
            LOG_INFOF(TAG, "🔄 Testing rotation %d", rotation);
            
            if (rotation >= 0 && rotation <= 3 && displayManager) {
                // Simple test: set rotation and clear screen with colored label
                displayManager->selectDisplay(1);
                TFT_eSPI* tft = displayManager->getTFT(1);
                if (tft) {
                    tft->setRotation(rotation);
                    tft->fillScreen(TFT_BLACK);
                    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
                    tft->setTextSize(3);
                    String label = "ROT " + String(rotation);
                    tft->drawString(label, 10, 40, 2);
                    displayManager->deselectAll();
                    
                    request->send(200, "text/plain", "Rotation " + String(rotation) + " applied - check display");
                } else {
                    request->send(500, "text/plain", "Display not available");
                }
            } else {
                request->send(400, "text/plain", "Invalid rotation (0-3) or display not available");
            }
        } else {
            // Show the web interface
            String html = "<html><head><title>Rotation Test</title></head><body>";
            html += "<h1>Rotation Tester</h1>";
            html += "<p>Click buttons to test rotations:</p>";
            html += "<button onclick=\"test(0)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 0</button>";
            html += "<button onclick=\"test(1)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 1</button>";
            html += "<button onclick=\"test(2)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 2</button>";
            html += "<button onclick=\"test(3)\" style=\"margin:10px;padding:20px;font-size:18px;\">ROT 3</button>";
            html += "<script>function test(r){fetch('/debug/rotation-test?rotation='+r).then(r=>r.text()).then(t=>alert(t));}</script>";
            html += "</body></html>";
            request->send(200, "text/html", html);
        }
    });
    
    LOG_INFO(TAG, "✅ Normal mode routes configured");
}

void WiFiManager::setupImageRoutes() {
    if (!imageManager) {
        LOG_WARN(TAG, "ImageManager not available - skipping image routes");
        return;
    }
    
    LOG_INFO(TAG, "🖼️ Setting up image management routes");
    
    // Enhanced upload endpoint to work with existing index.html
    server->on("/upload", HTTP_POST, 
        [this](AsyncWebServerRequest *request) {
            // This handles the response after file upload is complete
            Serial.println("=== UPLOAD REQUEST COMPLETE ===");
            request->send(200, "text/plain", "Upload completed successfully");
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            // Handle file upload data chunks
            static uint8_t* uploadBuffer = nullptr;
            static size_t totalSize = 0;
            static size_t receivedSize = 0;
            static String uploadFilename;
            
            Serial.printf("Upload chunk: %s, index=%d, len=%d, final=%s\n", 
                         filename.c_str(), index, len, final ? "YES" : "NO");
            
            if (index == 0) {
                // First chunk - initialize upload
                uploadFilename = filename;
                totalSize = request->contentLength();
                receivedSize = 0;
                
                Serial.println("=== UPLOAD START ===");
                Serial.println("Filename: " + filename);
                Serial.println("Total size: " + String(totalSize));
                
                LOG_INFOF(TAG, "📁 Starting image upload: %s (%d bytes)", filename.c_str(), totalSize);
                
                // Allocate buffer for entire file
                uploadBuffer = (uint8_t*)malloc(totalSize);
                if (!uploadBuffer) {
                    Serial.println("ERROR: Failed to allocate upload buffer");
                    LOG_ERROR(TAG, "Failed to allocate upload buffer");
                    request->send(500, "text/plain", "Memory allocation failed");
                    return;
                }
                Serial.println("Buffer allocated successfully");
            }
            
            // Copy chunk data to buffer
            if (uploadBuffer && (receivedSize + len <= totalSize)) {
                memcpy(uploadBuffer + receivedSize, data, len);
                receivedSize += len;
                Serial.printf("Copied chunk, received: %d/%d bytes\n", receivedSize, totalSize);
            } else {
                Serial.println("ERROR: Buffer issue or size mismatch");
            }
            
            if (final) {
                // Upload complete - process the image
                Serial.println("=== WIFI UPLOAD FINAL ===");
                Serial.println("Filename: " + uploadFilename);
                Serial.println("Expected size: " + String(totalSize));
                Serial.println("Received size: " + String(receivedSize));
                Serial.println("Size match: " + String(receivedSize == totalSize ? "YES" : "NO"));
                Serial.println("Buffer available: " + String(uploadBuffer ? "YES" : "NO"));
                Serial.println("ImageManager available: " + String(imageManager ? "YES" : "NO"));
                
                LOG_INFOF(TAG, "📁 Upload complete: %s (%d bytes)", uploadFilename.c_str(), receivedSize);
                
                if (uploadBuffer && imageManager) {
                    Serial.println("Calling handleImageUpload...");
                    // Use the total expected size, not received size for validation
                    bool success = imageManager->handleImageUpload(uploadFilename, uploadBuffer, totalSize);
                    Serial.println("handleImageUpload result: " + String(success ? "SUCCESS" : "FAILED"));
                    
                    if (!success) {
                        LOG_ERROR(TAG, "Image validation/save failed");
                        request->send(400, "text/plain", "Image validation failed - check format and size");
                    }
                } else {
                    Serial.println("ERROR: Missing buffer or ImageManager");
                    LOG_ERROR(TAG, "Upload buffer or ImageManager not available");
                    request->send(500, "text/plain", "Internal server error");
                }
                
                // Cleanup
                if (uploadBuffer) {
                    free(uploadBuffer);
                    uploadBuffer = nullptr;
                }
                totalSize = 0;
                receivedSize = 0;
            }
        });
    
    // Image system information API
    server->on("/api/images/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!imageManager) {
            request->send(500, "application/json", "{\"error\":\"ImageManager not available\"}");
            return;
        }
        
        String response = imageManager->getSystemInfo();
        request->send(200, "application/json", response);
    });
    
    // List all images API  
    server->on("/api/images/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
        LOG_INFO(TAG, "Image list API requested");
        
        if (!imageManager) {
            LOG_ERROR(TAG, "ImageManager not available for list request");
            request->send(500, "application/json", "{\"error\":\"ImageManager not available\"}");
            return;
        }
        
        String response = imageManager->getImageListJson();
        LOG_INFO(TAG, "Sending image list JSON response");
        request->send(200, "application/json", response);
    });
    
    // Display image on specific display
    server->on("/api/images/display", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!imageManager) {
            request->send(500, "application/json", "{\"error\":\"ImageManager not available\"}");
            return;
        }
        
        if (!request->hasParam("filename", true) || !request->hasParam("display", true)) {
            request->send(400, "application/json", "{\"error\":\"Missing filename or display parameter\"}");
            return;
        }
        
        String filename = request->getParam("filename", true)->value();
        int displayNum = request->getParam("display", true)->value().toInt();
        
        bool success = imageManager->displayImage(filename, displayNum);
        
        if (success) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"Failed to display image\"}");
        }
    });
    
    // Display image on both displays
    server->on("/api/images/display-both", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!imageManager) {
            request->send(500, "application/json", "{\"error\":\"ImageManager not available\"}");
            return;
        }
        
        if (!request->hasParam("filename", true)) {
            request->send(400, "application/json", "{\"error\":\"Missing filename parameter\"}");
            return;
        }
        
        String filename = request->getParam("filename", true)->value();
        bool success = imageManager->displayImageOnBoth(filename);
        
        if (success) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"Failed to display image\"}");
        }
    });
    
    // Delete image
    server->on("/api/images/delete", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
        if (!imageManager) {
            request->send(500, "application/json", "{\"error\":\"ImageManager not available\"}");
            return;
        }
        
        if (!request->hasParam("filename")) {
            request->send(400, "application/json", "{\"error\":\"Missing filename parameter\"}");
            return;
        }
        
        String filename = request->getParam("filename")->value();
        bool success = imageManager->deleteImage(filename);
        
        if (success) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"Failed to delete image\"}");
        }
    });
    
    // Get image enabled states for slideshow
    server->on("/api/images/enabled-states", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("=== ENABLED STATES REQUEST ===");
        if (!slideshowManager) {
            Serial.println("ERROR: SlideshowManager not available");
            request->send(500, "application/json", "{\"error\":\"SlideshowManager not available\"}");
            return;
        }
        
        auto enabledStates = slideshowManager->getImageEnabledStates();
        Serial.printf("Retrieved %d image states from slideshow manager\n", enabledStates.size());
        
        String response = "{\"states\":{";
        
        bool first = true;
        for (const auto& pair : enabledStates) {
            if (!first) response += ",";
            response += "\"" + pair.first + "\":" + (pair.second ? "true" : "false");
            Serial.printf("  %s = %s\n", pair.first.c_str(), pair.second ? "true" : "false");
            first = false;
        }
        
        response += "}}";
        Serial.printf("Sending response: %s\n", response.c_str());
        request->send(200, "application/json", response);
    });
    
    // Update image enabled state for slideshow
    server->on("/api/images/toggle-enabled", HTTP_POST, [this](AsyncWebServerRequest *request) {
        Serial.println("=== TOGGLE REQUEST RECEIVED ===");
        if (!request->hasParam("filename", true) || !request->hasParam("enabled", true)) {
            Serial.println("ERROR: Missing filename or enabled parameter");
            request->send(400, "application/json", "{\"error\":\"Missing filename or enabled parameter\"}");
            return;
        }
        
        String filename = request->getParam("filename", true)->value();
        String enabledStr = request->getParam("enabled", true)->value();
        bool enabled = (enabledStr == "true");
        
        Serial.printf("Toggle request: filename='%s', enabled='%s' -> %s\n", 
                     filename.c_str(), enabledStr.c_str(), enabled ? "TRUE" : "FALSE");
        
        // Update slideshow manager with the enabled state
        if (slideshowManager) {
            slideshowManager->updateImageEnabledState(filename, enabled);
            // Refresh the image list to apply the enabled/disabled state immediately
            slideshowManager->refreshImageList();
            Serial.println("Slideshow manager updated and refreshed");
        } else {
            Serial.println("ERROR: slideshowManager is null");
        }
        
        Serial.printf("Image %s %s for slideshow\n", filename.c_str(), enabled ? "enabled" : "disabled");
        
        request->send(200, "application/json", "{\"status\":\"success\"}");
    });
    
    // Serve image files for thumbnails
    server->on("/images/*", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!imageManager) {
            request->send(404, "text/plain", "ImageManager not available");
            return;
        }
        
        String path = request->url();
        String filename = path.substring(8); // Remove "/images/" prefix
        
        // Validate filename
        if (filename.length() == 0 || filename.indexOf("..") >= 0) {
            request->send(400, "text/plain", "Invalid filename");
            return;
        }
        
        // Check if file exists
        String imagePath = "/images/" + filename;
        if (!LittleFS.exists(imagePath)) {
            request->send(404, "text/plain", "Image not found");
            return;
        }
        
        // Send the image file
        request->send(LittleFS, imagePath, "image/jpeg");
    });
    
    LOG_INFO(TAG, "✅ Image management routes configured");
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

void WiFiManager::checkPortalModeSwitch() {
    if (switchToPortalMode) {
        LOG_INFO(TAG, "🌐 Switching to portal mode as requested via settings");
        switchToPortalMode = false;
        switchToSetupMode();
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

