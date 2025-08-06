#include "wifi_manager.h"
#include "logger.h"
#include "webcontent.h"

static const String TAG = "WIFI";

WiFiManager::WiFiManager(AsyncWebServer* webServer) : server(webServer) {
    LOG_DEBUG(TAG, "WiFiManager constructor called");
}

void WiFiManager::initializeAP(const String& ssid, const String& password) {
    apSSID = ssid;
    apPassword = password;
    
    LOG_INFO(TAG, "=== Starting Access Point ===");
    LOG_INFOF(TAG, "SSID: '%s'", ssid.c_str());
    LOG_INFOF(TAG, "Password: '%s'", password.c_str());
    
    // Stop any existing WiFi connections
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    LOG_DEBUG(TAG, "WiFi stopped, configuring AP mode...");
    
    // Configure custom IP for captive portal
    IPAddress local_IP(4, 3, 2, 1);
    IPAddress gateway(4, 3, 2, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    LOG_DEBUGF(TAG, "Setting AP IP to: %s", local_IP.toString().c_str());
    
    // Set AP mode
    WiFi.mode(WIFI_AP);
    
    // Configure AP with custom IP
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        LOG_ERROR(TAG, "❌ Failed to configure AP IP");
        return;
    }
    
    LOG_DEBUG(TAG, "AP IP configured, starting access point...");
    
    // Start the Access Point
    bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), 1, 0, 4);
    
    if (apStarted) {
        LOG_INFO(TAG, "✅ Access Point started successfully!");
        
        // Verify AP configuration immediately
        LOG_INFOF(TAG, "✅ AP SSID: %s", WiFi.softAPSSID().c_str());
        LOG_INFOF(TAG, "✅ AP IP: %s", WiFi.softAPIP().toString().c_str());
        LOG_INFOF(TAG, "✅ AP MAC: %s", WiFi.softAPmacAddress().c_str());
        LOG_INFOF(TAG, "✅ WiFi Mode: %d", WiFi.getMode());
        
        // Test if we can ping ourselves
        if (WiFi.softAPIP() == IPAddress(4, 3, 2, 1)) {
            LOG_INFO(TAG, "✅ AP IP confirmed as 4.3.2.1");
        } else {
            LOG_ERRORF(TAG, "❌ AP IP mismatch! Expected 4.3.2.1, got %s", WiFi.softAPIP().toString().c_str());
        }
        
    } else {
        LOG_ERROR(TAG, "❌ Failed to start Access Point!");
        LOG_ERROR(TAG, "Check if another AP is running or SSID is too long");
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
        
        String html = getPortalHTML();
        if (html.length() > 0) {
            LOG_INFOF(TAG, "✅ Serving portal HTML (%d bytes)", html.length());
            request->send(200, "text/html", html);
        } else {
            LOG_ERROR(TAG, "❌ Portal HTML not available!");
            request->send(500, "text/plain", "Portal HTML not found. Check webcontent.h file.");
        }
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
    // Limit to first 10 networks to prevent memory issues
    int maxNetworks = min(n, 10);
    
    for (int i = 0; i < maxNetworks; i++) {
        if (i > 0) networks += ",";
        
        // Escape quotes in SSID
        String ssid = WiFi.SSID(i);
        ssid.replace("\"", "\\\"");
        
        networks += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    networks += "]";
    
    // Clean up scan results to free memory
    WiFi.scanDelete();
    
    LOG_INFOF(TAG, "Found %d networks (showing %d)", n, maxNetworks);
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
            String response = "{\"status\":\"success\",\"message\":\"Successfully connected to " + ssid + 
                            "! IP: " + WiFi.localIP().toString() + "\"}";
            request->send(200, "application/json", response);
            LOG_INFOF(TAG, "✅ Connection successful - notified web interface");
        } else {
            String response = "{\"status\":\"error\",\"message\":\"Failed to connect to " + ssid + 
                            ". Check password and signal strength.\"}";
            request->send(400, "application/json", response);
            LOG_ERRORF(TAG, "❌ Connection failed - notified web interface");
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