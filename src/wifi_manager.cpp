#include "wifi_manager.h"
#include "logger.h"
#include "config.h"
#include "webcontent.h"
#include "display_manager.h"

static const String TAG = "WIFI";

const unsigned long WiFiManager::RETRY_DELAYS[] = {5000, 10000, 30000}; // 5s, 10s, 30s

WiFiManager::WiFiManager(AsyncWebServer* webServer, TimeManager* timeManager, SettingsManager* settingsManager, DisplayManager* displayManager) 
    : server(webServer), timeManager(timeManager), settingsManager(settingsManager), displayManager(displayManager) {
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
    bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), 1, 0, 4);
    
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
            
            // Save to persistent settings after response sent
            settingsManager->setDCCEnabled(isEnabled);
            LOG_INFOF(TAG, "🚂 DCC setting saved, current value: %s", settingsManager->isDCCEnabled() ? "true" : "false");
        } else {
            LOG_WARN(TAG, "⚠️ Missing dcc parameter");
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
    
    // File upload endpoint
    server->on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Upload endpoint not implemented yet");
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        // Handle file upload data
        LOG_INFOF(TAG, "📁 File upload: %s, chunk: %d bytes", filename.c_str(), len);
        // TODO: Implement file upload logic
    });
    
    // API endpoint for current settings
    server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        String response = "{";
        response += "\"secondDisplay\":" + String(settingsManager->isSecondDisplayEnabled() ? "true" : "false") + ",";
        response += "\"dcc\":" + String(settingsManager->isDCCEnabled() ? "true" : "false") + ",";
        response += "\"brightness\":" + String(settingsManager->getBrightness()) + ",";
        response += "\"imageInterval\":" + String(settingsManager->getImageInterval()) + ",";
        response += "\"imageEnabled\":" + String(settingsManager->isImageEnabled() ? "true" : "false");
        response += "}";
        request->send(200, "application/json", response);
    });

    // API endpoints for settings page
    server->on("/api/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = "{";
        response += "\"ssid\":\"" + WiFi.SSID() + "\",";
        response += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        response += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        response += "\"status\":\"" + String(WiFi.status() == WL_CONNECTED ? "connected" : "disconnected") + "\"";
        response += "}";
        request->send(200, "application/json", response);
    });
    
    server->on("/api/system-info", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = "{";
        response += "\"uptime\":" + String(millis() / 1000) + ",";
        response += "\"freeMemory\":" + String(ESP.getFreeHeap());
        response += "}";
        request->send(200, "application/json", response);
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

