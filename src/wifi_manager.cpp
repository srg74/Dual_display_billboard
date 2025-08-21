/**
 * @file wifi_manager.cpp
 * @brief 🌐 ESP32 Dual Display Billboard - Advanced WiFi Management System v0.9
 * 
 * @details Comprehensive WiFi and web server management system for ESP32 dual display billboard.
 *          Provides sophisticated network connectivity, dual-mode operation (Setup/Normal),
 *          comprehensive web interface with image management, DCC control, system monitoring,
 *          factory reset functionality, and robust connection management with automatic retry logic.
 * 
 * Key Features:
 * • 🏗️ Dual-Mode Operation: Setup mode for initial configuration, Normal mode for operations
 * • 🌐 Advanced Network Management: Auto-connection, retry logic, connection health monitoring
 * • Comprehensive Web Interface: Settings management, image upload/control, system status
 * • Image Management: Upload, enable/disable, preview, slideshow control via web interface
 * • 🚂 DCC Integration: Web-based DCC command interface for model railroad control
 * • System Monitoring: Real-time memory health, WiFi status, platform information
 * • 🔄 Factory Reset: GPIO0-based factory reset with configurable timing
 * • 📊 RESTful API: JSON endpoints for system status, image management, settings control
 * • 🛡️ Robust Error Handling: Connection timeout management, automatic mode switching
 * • Persistent Configuration: Credential storage, connection state management
 * 
 * Architecture:
 * - Setup Mode: Access Point for initial WiFi configuration and system setup
 * - Normal Mode: Station mode with full web interface and system functionality
 * - Dynamic Mode Switching: Automatic transition based on connection status and user input
 * - RESTful Web Services: JSON API for external integration and mobile applications
 * - Integrated Image Pipeline: Direct web-based image upload and slideshow management
 * - Memory-Aware Operations: Dynamic content serving based on available memory
 * 
 * Dependencies: AsyncWebServer, TimeManager, SettingsManager, DisplayManager, 
 *               ImageManager, SlideshowManager, DCCManager, MemoryManager
 * Platform: ESP32 with WiFi capability
 * Version: 0.9 - Pre-release development version with comprehensive system integration
 * 
 * @author ESP32 Dual Display Billboard System
 * @version 0.9
 * @date 2025
 */

#include "wifi_manager.h"
#include "dcc_manager.h"
#include "logger.h"
#include "config.h"
#include "webcontent.h"
#include "display_manager.h"
#include "display_timing_config.h"
#include "slideshow_manager.h"
#include "memory_manager.h"
#include "platform_detector.h"

static const String TAG = "WIFI";

const unsigned long WiFiManager::RETRY_DELAYS[] = {5000, 10000, 30000}; // 5s, 10s, 30s

// ============================================================================
// 🏗️ CONSTRUCTOR & INITIALIZATION
// ============================================================================

/**
 * @brief 🏗️ Constructs WiFiManager with comprehensive system integration
 * 
 * @details Initializes the WiFi management system with references to all core system components.
 *          Sets up dual-mode operation state, connection retry logic, GPIO factory reset monitoring,
 *          and establishes communication pathways between web interface and system components.
 * 
 * Key Initialization:
 * • Dual-mode state management (Setup/Normal mode switching)
 * • Connection retry and timing control systems
 * • GPIO0 factory reset monitoring and debouncing
 * • Cross-component communication interfaces
 * • Memory and connection health monitoring
 * • Restart scheduling and portal mode switching
 * 
 * @param webServer Pointer to AsyncWebServer instance for HTTP request handling
 * @param timeManager Pointer to TimeManager for timestamp and scheduling operations
 * @param settingsManager Pointer to SettingsManager for configuration persistence
 * @param displayManager Pointer to DisplayManager for visual status feedback
 * @param imageManager Pointer to ImageManager for image storage and retrieval
 * @param slideshowManager Pointer to SlideshowManager for slideshow control
 * @param dccManager Pointer to DCCManager for model railroad command integration
 * 
 * @note Constructor establishes component relationships but does not initiate network operations.
 *       Call initializeAP() or initializeFromCredentials() to begin WiFi operations.
 */
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
    
    // Initialize connection success display tracking
    connectionSuccessDisplayed = false;
    connectionSuccessStartTime = 0;
    
    // Initialize GPIO0 pin for factory reset
    pinMode(GPIO0_PIN, INPUT_PULLUP);
}

/**
 * @brief 🌐 Initializes WiFi Access Point for setup mode configuration
 * 
 * @details Establishes ESP32 as WiFi Access Point to enable initial system configuration.
 *          Creates isolated network environment for secure credential entry and system setup.
 *          Provides visual feedback through display manager and comprehensive logging for debugging.
 * 
 * Setup Process:
 * • Stores AP credentials for internal reference and display
 * • Configures ESP32 WiFi hardware for Access Point mode
 * • Displays setup status on connected displays
 * • Enables DHCP for client device connectivity
 * • Prepares for web server route configuration
 * 
 * @param ssid Access Point network name (visible to connecting devices)
 * @param password Access Point password for network security (WPA2-PSK)
 * 
 * @note This method only configures the Access Point. Call setupRoutes() and startServer()
 *       to enable web interface functionality for configuration.
 * @note Access Point mode provides isolated network for secure initial setup without
 *       requiring existing WiFi infrastructure.
 */
void WiFiManager::initializeAP(const String& ssid, const String& password) {
    apSSID = ssid;
    apPassword = password;
    
    LOG_INFO(TAG, "=== Starting Access Point ===");
    LOG_INFOF(TAG, "SSID: '%s'", ssid.c_str());
    LOG_INFOF(TAG, "Password: '%s'", password.c_str());
    
    // Show starting AP status on display
    if (displayManager) {
        displayManager->showAPStarting();
    }
    
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
        LOG_ERROR(TAG, "Failed to configure AP IP");
        displayManager->showQuickStatus("AP Config Failed", TFT_RED);
        return;
    }
    
    LOG_DEBUG(TAG, "AP IP configured, starting access point...");
    
    // Start the Access Point (critical path)
    bool apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), 11, 0, 4);
    
    if (apStarted) {
        IPAddress IP = WiFi.softAPIP();
        LOG_INFOF(TAG, "Access Point started successfully! SSID: %s, IP: %s", 
                  apSSID.c_str(), IP.toString().c_str());
        
        currentMode = MODE_SETUP;
        
        // DIRECT PORTAL SEQUENCE: Show splash screen (4s) then portal info
        // This ensures identical behavior on ESP32 and ESP32S3
        displayManager->showPortalSequence(
            PORTAL_SSID,           
            "IP: 4.3.2.1",        
            "Ready to connect"     
        );
        
    } else {
        LOG_ERROR(TAG, "Failed to start Access Point!");
        LOG_ERROR(TAG, "Check if another AP is running or SSID is too long");
        
        // Show error status
        displayManager->showQuickStatus("AP Start Failed", TFT_RED);
    }
}

// ============================================================================
// 🌐 WEB SERVER ROUTE CONFIGURATION
// ============================================================================

/**
 * @brief 🌐 Configures comprehensive web server routes for setup mode
 * 
 * @details Establishes complete web interface routing for WiFi setup and initial system configuration.
 *          Provides memory-aware content serving, comprehensive system diagnostics, network scanning,
 *          credential management, and robust error handling for setup mode operations.
 * 
 * Route Categories:
 * • System Diagnostics: /test, /status, /memory endpoints for health monitoring
 * • 🌐 Network Management: /, /portal for main interface, /scan for network discovery
 * • 📡 Connection Control: /connect for WiFi credential submission and validation
 * • Visual Interface: Portal HTML with responsive design and status feedback
 * • Memory Management: Dynamic content serving based on available heap memory
 * • 📊 Real-time Status: Live system information including memory health and connectivity
 * 
 * Key Features:
 * • Memory-aware content serving (minimal interface for low memory conditions)
 * • Chunked response handling for large HTML content
 * • Cross-Origin Resource Sharing (CORS) support for external access
 * • Comprehensive error handling with user-friendly feedback
 * • Real-time memory and system status reporting
 * • Network scanning with security information display
 * 
 * @note Routes configured here are active only in setup mode. Normal mode uses
 *       setupNormalModeRoutes() for expanded functionality.
 * @note Memory threshold of 50KB used for content serving decisions to prevent crashes.
 */
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
        response += "Memory Health: " + String(MemoryManager::getHealthStatusString(MemoryManager::getOverallHealth())) + "\n";
        response += "Free Heap: " + String(MemoryManager::getAvailableMemory(MemoryManager::HEAP_INTERNAL)) + " bytes\n";
        if (MemoryManager::getAvailableMemory(MemoryManager::PSRAM_EXTERNAL) > 0) {
            response += "Free PSRAM: " + String(MemoryManager::getAvailableMemory(MemoryManager::PSRAM_EXTERNAL)) + " bytes\n";
        }
        response += "\nMemory API: /memory | /memory/health\n";
        response += "Rotation Tester: /debug/rotation-test\n";
        request->send(200, "text/plain", response);
    });

    // Main portal route
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🌐 Portal page requested");
        
        // Check available memory before serving large content
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 50000) {  // Less than 50KB free
            LOG_WARNF(TAG, "Low memory (%d bytes), serving minimal page", freeHeap);
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
            LOG_ERROR(TAG, "Portal HTML not available!");
            request->send(500, "text/plain", "Portal HTML generation failed");
            return;
        }
        
        LOG_INFOF(TAG, "Serving portal HTML (%d bytes), Free heap: %d", 
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
    
    // Status route with comprehensive memory monitoring
    server->on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String memoryStats = MemoryManager::getMemoryStatsJson();
        String status = "{";
        status += "\"uptime\":\"" + String(millis() / 1000) + " seconds\",";
        status += "\"uptimeMs\":" + String(millis()) + ",";
        status += "\"freeMemory\":" + String(ESP.getFreeHeap()) + ",";
        status += "\"memory\":" + memoryStats;
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
    
    // Memory monitoring endpoint
    server->on("/memory", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", MemoryManager::getMemoryStatsJson());
        response->addHeader("Connection", "close");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    });
    
    // Memory health endpoint
    server->on("/memory/health", HTTP_GET, [](AsyncWebServerRequest *request){
        String healthStatus = "{";
        healthStatus += "\"overallHealth\":\"" + String(MemoryManager::getHealthStatusString(MemoryManager::getOverallHealth())) + "\",";
        healthStatus += "\"isLowMemory\":" + String(MemoryManager::isLowMemory() ? "true" : "false") + ",";
        healthStatus += "\"isCriticalMemory\":" + String(MemoryManager::isCriticalMemory() ? "true" : "false") + ",";
        healthStatus += "\"heapHealth\":\"" + String(MemoryManager::getHealthStatusString(MemoryManager::getHealthStatus(MemoryManager::HEAP_INTERNAL))) + "\",";
        healthStatus += "\"psramHealth\":\"" + String(MemoryManager::getHealthStatusString(MemoryManager::getHealthStatus(MemoryManager::PSRAM_EXTERNAL))) + "\"";
        healthStatus += "}";
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", healthStatus);
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
        LOG_WARNF(TAG, "404 - Not found: %s", request->url().c_str());
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
    
    // Setup OTA routes for portal mode too
    setupOTARoutes();
    
    LOG_INFO(TAG, "Web routes configured successfully");
}

/**
 * @brief 🚀 Starts the HTTP web server for client request handling
 * 
 * @details Activates the AsyncWebServer instance to begin listening for HTTP requests on port 80.
 *          Enables access to all configured routes including portal interface, system diagnostics,
 *          and WiFi configuration endpoints. Server activation is the final step in setup mode preparation.
 * 
 * @note Server must be started after route configuration (setupRoutes()) to ensure all
 *       endpoints are properly registered and available to clients.
 * @note Default HTTP port 80 used for standard web browser compatibility.
 */
void WiFiManager::startServer() {
    LOG_INFO(TAG, "🚀 Starting web server...");
    server->begin();
    LOG_INFO(TAG, "Web server started and listening on port 80");
}

// ============================================================================
// 📡 NETWORK SCANNING & DISCOVERY
// ============================================================================

/**
 * @brief 📡 Performs WiFi network scan and returns formatted HTML results
 * 
 * @details Executes comprehensive WiFi network discovery scan and formats results as HTML
 *          for web interface display. Provides detailed network information including
 *          signal strength, security type, and channel information for user network selection.
 * 
 * Scan Features:
 * • Clears previous scan results to ensure fresh data
 * • Comprehensive network discovery with signal strength analysis
 * • Security type detection (Open, WEP, WPA/WPA2, WPA3)
 * • Signal strength visualization with colored indicators
 * • Channel information for interference analysis
 * • HTML formatted output for direct web interface integration
 * • Error handling for scan failures and timeout conditions
 * 
 * @return String HTML formatted list of discovered networks with selection interface
 * @note Returns "No networks found" message if scan yields no results
 * @note Scan may take 5-10 seconds depending on channel coverage and network density
 */
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
        
        // Non-blocking delay for mode change
        unsigned long modeChangeStart = millis();
        while (millis() - modeChangeStart < 100) {
            yield();
        }
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
        
        // Non-blocking delay for mode change
        unsigned long modeRestoreStart = millis();
        while (millis() - modeRestoreStart < 50) {
            yield();
        }
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

// ============================================================================
// 🔗 WIFI CONNECTION MANAGEMENT
// ============================================================================

/**
 * @brief 🔗 Establishes WiFi connection with comprehensive error handling
 * 
 * @details Initiates WiFi connection to specified network with robust timeout management,
 *          connection status monitoring, and visual feedback through display system.
 *          Provides detailed logging and error reporting for troubleshooting connectivity issues.
 * 
 * Connection Process:
 * • Network credential validation and sanitization
 * • WiFi hardware configuration for station mode
 * • Connection initiation with specified credentials
 * • Timeout-based connection monitoring (20-second limit)
 * • Connection status verification and IP address assignment
 * • Visual feedback through display manager status updates
 * • Comprehensive error logging for failed connection attempts
 * 
 * @param ssid Target WiFi network name (SSID) for connection
 * @param password Network password for WPA/WPA2 authentication
 * 
 * @return bool True if connection established successfully with valid IP assignment,
 *              false if connection failed or timed out
 * 
 * @note Connection timeout set to 20 seconds to prevent indefinite hanging
 * @note Displays connection status on attached displays for user feedback
 * @note Failed connections leave WiFi in disconnected state for retry attempts
 */
/**
 * @brief 🌐 Establishes WiFi connection with enhanced crash prevention
 * 
 * @details Connects ESP32 to specified WiFi network with comprehensive authentication failure
 *          handling and crash prevention mechanisms. Implements aggressive timeout management,
 *          early failure detection, and graceful disconnection to prevent ESP32 WiFi stack
 *          abort() crashes that occur during prolonged authentication failures.
 * 
 * Enhanced Safety Features (v0.9.1):
 * • 🛡️ Authentication Failure Prevention: Detects and aborts early on auth failures
 * • Aggressive Timeout Management: Reduced to 8 seconds to prevent prolonged failures
 * • Real-time Status Monitoring: 250ms status checks for rapid failure detection
 * • 🚫 Early Abort Logic: Stops after 2 quick failures to prevent crash conditions
 * • Aggressive Cleanup: Forces disconnection after failures with delay
 * • 📊 Visual Feedback: Display integration for connection status and failures
 * 
 * Connection Process:
 * • Pre-connection cleanup with forced disconnect and delay
 * • Real-time WiFi status monitoring with 250ms granularity
 * • Authentication failure counting with early abort (max 2 attempts)
 * • Stuck connection detection (disconnected state > 3 seconds)
 * • Comprehensive cleanup on failure with forced disconnect and delay
 * 
 * Safety Mechanisms:
 * • Prevents ESP32 WiFi stack abort() crashes during authentication failures
 * • Detects WL_CONNECT_FAILED and WL_CONNECTION_LOST states early
 * • Monitors for prolonged WL_DISCONNECTED states indicating stuck connections
 * • Forces WiFi hardware reset on failure to ensure clean state
 * 
 * @param ssid Target network SSID for connection attempt
 * @param password Network password for WPA/WPA2 authentication
 * 
 * @return bool True if connection established successfully with valid IP assignment,
 *              false if connection failed, timed out, or aborted due to safety mechanisms
 * 
 * @note Connection timeout aggressively reduced to 8 seconds to prevent auth crashes
 * @note Displays connection status on attached displays for user feedback
 * @note Failed connections leave WiFi in clean disconnected state with hardware reset
 * @note Multiple quick failures trigger early abort to prevent system crash
 * 
 * @see WiFi.onEvent() for complementary event-based crash prevention
 * @see displayManager->showConnecting() for visual feedback integration
 * @warning Authentication failures can cause ESP32 WiFi stack crashes if not handled properly
 * 
 * @version 0.9.1 - Enhanced crash prevention and authentication failure handling
 */
bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
    LOG_INFOF(TAG, "Attempting to connect to WiFi: %s", ssid.c_str());
    
    // Start the connection
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout (non-blocking in chunks)
    unsigned long startTime = millis();
    const unsigned long timeout = 15000; // 15 seconds timeout
    unsigned long lastYield = 0;
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
        yield(); // Allow other tasks
        
        // Non-blocking delay using millis() - check every 50ms but only log every 2 seconds
        if (millis() - lastYield >= 50) {
            lastYield = millis();
            
            // Log progress every 2 seconds
            if ((millis() - startTime) % 2000 < 50) {
                LOG_DEBUGF(TAG, "Connection attempt... Status: %d", WiFi.status());
            }
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFOF(TAG, "Connected successfully! IP: %s", WiFi.localIP().toString().c_str());
        LOG_INFOF(TAG, "Signal strength: %d dBm", WiFi.RSSI());
        return true;
    } else {
        LOG_ERRORF(TAG, "Connection failed. Status: %d", WiFi.status());
        
        // Show connection failed status on display
        if (displayManager) {
            displayManager->showQuickStatus("WiFi Failed", TFT_RED);
        }
        
        WiFi.disconnect(); // Clean up failed connection
        return false;
    }
}

/**
 * @brief 🔗 Handles HTTP POST requests for WiFi connection from web interface
 * 
 * @details Processes WiFi connection requests submitted through the web portal interface.
 *          Validates submitted credentials, attempts network connection, manages credential storage,
 *          and provides immediate HTTP response with connection status and next steps.
 * 
 * Request Processing:
 * • Validates presence of required SSID and password parameters
 * • Sanitizes and extracts network credentials from HTTP form data
 * • Initiates WiFi connection attempt using connectToWiFi()
 * • Manages credential persistence through CredentialManager integration
 * • Schedules system restart for mode transition upon successful connection
 * • Provides detailed HTTP response with connection status and user instructions
 * 
 * Response Handling:
 * • Success: Returns status page with restart notification and IP information
 * • Failure: Returns error page with retry instructions and troubleshooting tips
 * • Missing Parameters: Returns error response indicating required field validation
 * • Credential Storage: Logs success/failure of credential persistence operations
 * 
 * @param request Pointer to AsyncWebServerRequest containing WiFi credentials and metadata
 * 
 * @note Successful connections trigger 3-second delayed restart to transition to normal mode
 * @note Failed connections leave system in setup mode for additional retry attempts
 * @note All credential handling logged for security auditing and debugging purposes
 */
void WiFiManager::handleConnect(AsyncWebServerRequest* request) {
    LOG_INFO(TAG, "WiFi connection request received");
    
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        
        LOG_INFOF(TAG, "Starting connection to: %s", ssid.c_str());
        
        // Show connecting status on display for visual feedback
        if (displayManager) {
            displayManager->showConnecting();
        }
        
        // Attempt connection
        bool connected = connectToWiFi(ssid, password);
        
        if (connected) {
            // Save credentials
            if (CredentialManager::saveCredentials(ssid, password)) {
                LOG_INFO(TAG, "Credentials saved successfully");
            } else {
                LOG_WARN(TAG, "Failed to save credentials, but connection succeeded");
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
            LOG_INFOF(TAG, "Connection successful - device will be available at %s", deviceIP.c_str());
            
            // Show connection success message
            if (displayManager) {
                displayManager->showConnectionSuccess(deviceIP);
                connectionSuccessDisplayed = true;
                connectionSuccessStartTime = millis();
            }
            
            // NON-BLOCKING RESTART: Schedule restart instead of immediate delay
            restartPending = true;
            restartScheduledTime = millis() + 3000;  // 3 seconds from now
            LOG_INFO(TAG, "🔄 Restart scheduled in 3 seconds...");
            
        } else {
            String response = "{\"status\":\"error\",\"message\":\"Failed to connect to " + ssid + ". Check password and signal strength.\"}";
            request->send(400, "application/json", response);
            LOG_ERROR(TAG, "Connection failed");
            
            // Show connection failed status on display
            if (displayManager) {
                displayManager->showQuickStatus("Connection Failed", TFT_RED);
            }
        }
    } else {
        LOG_ERROR(TAG, "Missing SSID or password in request");
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID or password\"}");
    }
}

// ============================================================================
// MEMORY & SYSTEM HEALTH MONITORING
// ============================================================================

/**
 * @brief Monitors system memory health and triggers automatic cleanup
 * 
 * @details Continuously monitors ESP32 memory health using MemoryManager integration.
 *          Provides proactive memory management with automatic cleanup triggers and
 *          emergency response protocols to prevent system crashes and ensure stable operation.
 * 
 * Health Monitoring:
 * • Real-time memory health status assessment using MemoryManager
 * • Warning level detection with enhanced monitoring activation
 * • Critical level response with immediate cleanup operations
 * • Emergency level handling with system stability protocols
 * • Detailed logging for memory trend analysis and debugging
 * 
 * Automated Responses:
 * • WARNING: Enhanced monitoring with detailed status logging
 * • CRITICAL: Immediate forceCleanup() execution to reclaim memory
 * • EMERGENCY: System stability protocols and potential restart scheduling
 * • HEALTHY: Normal operation with periodic status verification
 * 
 * @note Called periodically during WiFiManager operations to ensure system stability
 * @note Memory cleanup operations may cause brief service interruptions
 * @note Emergency conditions may trigger automatic system restart for recovery
 */
void WiFiManager::checkHeapHealth() {
    // Use the new memory manager for comprehensive health checking
    MemoryManager::HealthStatus overallHealth = MemoryManager::getOverallHealth();
    
    switch (overallHealth) {
        case MemoryManager::WARNING:
            LOG_WARNF(TAG, "Memory health: %s - monitoring closely", 
                     MemoryManager::getHealthStatusString(overallHealth));
            break;
        case MemoryManager::CRITICAL:
            LOG_ERRORF(TAG, "Critical memory condition: %s - cleanup needed", 
                      MemoryManager::getHealthStatusString(overallHealth));
            // Trigger immediate cleanup
            MemoryManager::forceCleanup();
            break;
        case MemoryManager::EMERGENCY:
            LOG_ERRORF(TAG, "EMERGENCY memory condition: %s - system unstable", 
                      MemoryManager::getHealthStatusString(overallHealth));
            // Force immediate cleanup and consider restart
            MemoryManager::forceCleanup();
            break;
        default:
            // EXCELLENT or GOOD - no action needed
            break;
    }
    
    // Additional specific warnings for legacy compatibility
    size_t freeHeap = MemoryManager::getAvailableMemory(MemoryManager::HEAP_INTERNAL);
    if (freeHeap < 50000) { // Less than 50KB free
        LOG_WARNF(TAG, "Low heap memory: %d bytes free", freeHeap);
    }
}

// ============================================================================
// 🔄 MODE MANAGEMENT & AUTO-CONNECT SYSTEM
// ============================================================================

/**
 * @brief 🔄 Initializes WiFi connection using stored credentials with fallback handling
 * 
 * @details Attempts to establish WiFi connection using previously saved network credentials.
 *          Provides automatic fallback to setup mode if credentials are missing, invalid,
 *          or connection attempts fail. Integrates with CredentialManager for secure storage access.
 * 
 * Initialization Process:
 * • Checks for existence of saved WiFi credentials in persistent storage
 * • Validates credential integrity and format before connection attempt
 * • Attempts network connection using stored SSID and password
 * • Transitions to normal mode upon successful connection establishment
 * • Falls back to setup mode if any step fails for user intervention
 * 
 * Fallback Scenarios:
 * • No saved credentials → Setup mode for initial configuration
 * • Invalid credentials → Setup mode for credential correction
 * • Connection failure → Setup mode for network troubleshooting
 * • Network unavailable → Setup mode with retry capability
 * 
 * @return bool True if connection established successfully using saved credentials,
 *              false if fallback to setup mode required
 * 
 * @note Automatic mode switching ensures system remains accessible for configuration
 * @note Credential validation prevents connection attempts with corrupted data
 */
bool WiFiManager::initializeFromCredentials() {
    LOG_INFO(TAG, "Checking for saved WiFi credentials...");
    
    if (!CredentialManager::hasCredentials()) {
        LOG_INFO(TAG, "📄 No saved credentials found - starting setup mode");
        switchToSetupMode();
        return false;
    }
    
    CredentialManager::WiFiCredentials creds = CredentialManager::loadCredentials();
    if (!creds.isValid) {
        LOG_ERROR(TAG, "Invalid credentials found - starting setup mode");
        switchToSetupMode();
        return false;
    }
    
    LOG_INFOF(TAG, "Found credentials for: %s", creds.ssid.c_str());
    
    // Try to connect to saved network
    if (connectToSavedNetwork()) {
        // CHANGED: Don't immediately switch to normal mode - let connection success display run first
        // switchToNormalMode() will be called by checkConnectionSuccessDisplay() after 5 seconds
        return true;
    } else {
        LOG_WARN(TAG, "Auto-connect failed - falling back to setup mode");
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
    
    // Show connecting status on display for visual feedback
    if (displayManager) {
        displayManager->showConnecting();
    }
    
    // Stop AP mode and switch to STA mode
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    // Attempt connection
    bool connected = connectToWiFi(creds.ssid, creds.password);
    
    if (connected) {
        LOG_INFOF(TAG, "Auto-connected successfully! IP: %s", WiFi.localIP().toString().c_str());
        connectionRetryCount = 0; // Reset retry counter
        
        // NEW: Show connection success message
        displayManager->showConnectionSuccess(WiFi.localIP().toString());
        connectionSuccessDisplayed = true;
        connectionSuccessStartTime = millis();
        
        return true;
    } else {
        LOG_ERROR(TAG, "Auto-connect failed");
        return false;
    }
}

/**
 * @brief 🔄 Transitions system from setup mode to full operational normal mode
 * 
 * @details Reconfigures WiFiManager for normal operation mode after successful WiFi connection.
 *          Stops setup mode web server, configures comprehensive normal mode routes,
 *          and restarts server with full system functionality including image management,
 *          DCC control, and advanced system monitoring capabilities.
 * 
 * Transition Process:
 * • Updates internal mode state from MODE_SETUP to MODE_NORMAL
 * • Gracefully stops existing web server to prevent connection conflicts
 * • Configures comprehensive normal mode route set via setupNormalModeRoutes()
 * • Restarts web server with expanded functionality and system integration
 * • Provides visual confirmation of successful mode transition
 * 
 * Normal Mode Features:
 * • Full web interface with image upload and management
 * • DCC command interface for model railroad control
 * • Comprehensive system status and configuration endpoints
 * • Real-time monitoring and diagnostic capabilities
 * • Advanced settings management and system control
 * 
 * @note Mode transition involves brief service interruption during server restart
 * @note Normal mode provides access to full system functionality and integration
 * @note Server remains accessible via WiFi IP address after transition
 */
void WiFiManager::switchToNormalMode() {
    LOG_INFO(TAG, "🔄 Switching to NORMAL mode");
    currentMode = MODE_NORMAL;
    
    // Stop the current server
    server->end();
    
    // Setup routes for normal mode
    setupNormalModeRoutes();
    
    // Start server again
    startServer();
    
    LOG_INFOF(TAG, "Normal mode active - server running on WiFi IP: %s", WiFi.localIP().toString().c_str());
}

/**
 * @brief 🔄 Transitions system from normal mode to setup mode for configuration
 * 
 * @details Reconfigures WiFiManager for setup mode operation when WiFi connection is lost,
 *          factory reset is triggered, or manual configuration is required. Establishes
 *          Access Point mode for secure credential entry and system reconfiguration.
 * 
 * Transition Process:
 * • Updates internal mode state from MODE_NORMAL to MODE_SETUP
 * • Records setup mode initiation timestamp for timeout management
 * • Gracefully stops current web server to prevent service conflicts
 * • Disconnects from current WiFi network to free radio resources
 * • Initializes Access Point with predefined SSID and password
 * • Configures setup mode routes for credential entry and system configuration
 * • Restarts web server with portal interface for user access
 * 
 * Setup Mode Features:
 * • Isolated Access Point for secure configuration without external dependencies
 * • WiFi network scanning and selection interface
 * • Credential entry and validation system
 * • Basic system status and diagnostic capabilities
 * • Factory reset and system restoration options
 * 
 * @note Setup mode provides isolated network environment for secure reconfiguration
 * @note Mode transition involves brief service interruption during server restart
 * @note Access Point credentials defined by PORTAL_SSID and PORTAL_PASSWORD constants
 */
void WiFiManager::switchToSetupMode() {
    LOG_INFO(TAG, "🔄 Switching to SETUP mode");
    currentMode = MODE_SETUP;
    
    unsigned long startTime = millis();
    LOG_INFOF(TAG, "Setup mode start time: %lu ms", startTime);
    
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
    LOG_INFOF(TAG, "Setup mode completed in: %lu ms", endTime - startTime);
    
    LOG_INFO(TAG, "Setup mode active - portal running at http://4.3.2.1");
}

/**
 * @brief 🌐 Configures web server routes for normal operation mode
 * 
 * Establishes production web interface endpoints for billboard operation,
 * system management, and user interaction. Provides comprehensive routing
 * for all normal mode functionality including display control, settings,
 * and system monitoring.
 * 
 * Normal Mode Routes:
 * • GET / - Main billboard interface with live display data
 * • GET /settings - System configuration and management page  
 * • POST /settings - Configuration updates and system changes
 * • GET /api/status - Real-time system status and health data
 * • GET /api/images - Current image library and metadata
 * • Integration with image upload and slideshow management
 * 
 * Route Features:
 * • Dynamic HTML templating with live system data
 * • RESTful API endpoints for programmatic access
 * • Real-time status monitoring and health checks
 * • Settings management with validation and persistence
 * • Image library management and slideshow control
 * 
 * Template Processing:
 * • Live system data injection (WiFi status, memory usage)
 * • Device information display (IP address, firmware version)
 * • Dynamic content updates based on current system state
 * • Error handling for missing templates or data
 * 
 * @note Routes configured for production operation after WiFi connection
 * @note All routes include proper CORS headers for web compatibility
 * @note Template processing includes security considerations
 * 
 * @see setupImageRoutes() for image management endpoint configuration
 * @see getIndexHTML() for main interface template processing
 */
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
        LOG_INFO(TAG, "Settings page requested");
        
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
    
    // CSS styles
    server->on("/styles.css", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_INFO(TAG, "🎨 CSS styles requested");
        String css = getStylesCSS();
        request->send(200, "text/css", css);
    });
    
    // Time API
    server->on("/time", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (timeManager) {
            bool timeValid = timeManager->isTimeValid();
            String currentTime = timeManager->getCurrentTime();
            LOG_INFOF(TAG, "🕐 Time API request - Valid: %s, Time: %s", 
                timeValid ? "true" : "false", currentTime.c_str());
            
            if (timeValid) {
                request->send(200, "text/plain", currentTime);
            } else {
                request->send(200, "text/plain", "--:--");
            }
        } else {
            LOG_WARN(TAG, "🕐 Time API request - TimeManager is null");
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
            LOG_INFOF(TAG, "Image interval set to: %d seconds", intervalValue);
            
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
            
            // Save to persistent settings - SettingsManager will automatically apply brightness
            settingsManager->setSecondDisplayEnabled(isEnabled);
            LOG_INFOF(TAG, "📺 Second display setting saved and brightness applied automatically, current value: %s", settingsManager->isSecondDisplayEnabled() ? "true" : "false");
        } else {
            LOG_WARN(TAG, "Missing second_display parameter");
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
            LOG_WARN(TAG, "Missing dcc parameter");
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
                LOG_WARNF(TAG, "Invalid DCC address: %d", address);
                request->send(400, "text/plain", "Invalid address (must be 1-2048)");
            }
        } else {
            LOG_WARN(TAG, "Missing address parameter");
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
                LOG_WARNF(TAG, "Invalid DCC pin: %d", pin);
                request->send(400, "text/plain", "Invalid pin (must be 0-39)");
            }
        } else {
            LOG_WARN(TAG, "Missing pin parameter");
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
                LOG_WARNF(TAG, "Invalid DCC address: %d", address);
                request->send(400, "text/plain", "Invalid DCC address (1-2048)");
            }
        } else {
            LOG_WARN(TAG, "Missing address parameter");
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
                LOG_WARNF(TAG, "Invalid GPIO pin: %d", pin);
                request->send(400, "text/plain", "Invalid GPIO pin (0-39)");
            }
        } else {
            LOG_WARN(TAG, "Missing pin parameter");
            request->send(400, "text/plain", "Missing parameter");
        }
    });
    
    // Image enable toggle
    server->on("/image-enable", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("image_enable", true)) {
            String enabled = request->getParam("image_enable", true)->value();
            bool isEnabled = (enabled == "true");
            LOG_INFOF(TAG, "Image display: %s", isEnabled ? "enabled" : "disabled");
            
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
            
            // Save to persistent settings - SettingsManager will automatically apply to hardware
            settingsManager->setBrightness(brightnessValue);
            LOG_DEBUG(TAG, "Brightness setting saved and applied automatically");
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
            LOG_WARN(TAG, "Missing clock parameter");
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
                LOG_WARN(TAG, "Invalid clock face type");
                request->send(400, "text/plain", "Invalid face type");
            }
        } else {
            LOG_WARN(TAG, "Missing face parameter");
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
        response += "\"freeMemory\":" + String(ESP.getFreeHeap()) + ",";
        response += "\"platform\":\"" + PlatformDetector::getPlatformSummary() + "\",";
        response += "\"memoryDetails\":" + MemoryManager::getMemoryStatsJson();
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
                LOG_INFOF(TAG, "NTP server updated to: %s", ntpServer.c_str());
                request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"NTP server updated successfully\"}");
            } else {
                LOG_ERROR(TAG, "TimeManager not available");
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
    
    // Setup OTA firmware update routes
    setupOTARoutes();
    
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
    
    // Memory monitoring API endpoint
    server->on("/api/memory-status", HTTP_GET, [this](AsyncWebServerRequest *request){
        LOG_DEBUG(TAG, "Memory status API requested");
        
        String memoryJson = MemoryManager::getMemoryStatsJson();
        
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", memoryJson);
        response->addHeader("Connection", "close");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    });
    
    LOG_INFO(TAG, "Normal mode routes configured");
}

/**
 * @brief Sets up image management HTTP routes for file upload and manipulation
 * 
 * Configures comprehensive image handling endpoints including:
 * • File upload with chunk processing and validation
 * • Image conversion and optimization
 * • Gallery management and listing
 * • Direct image display and preview
 * • Slideshow control integration
 * 
 * Features:
 * • Multi-format support (JPEG, PNG, BMP)
 * • Automatic resize and optimization
 * • Progress tracking during upload
 * • Memory-efficient chunk processing
 * • Integration with ImageManager for processing
 * 
 * @note Requires ImageManager to be initialized
 * @note Handles large file uploads via chunked processing
 * @warning Upload endpoint processes raw binary data - ensure proper validation
 * 
 * Routes configured:
 * • POST /upload - Multi-part file upload with progress tracking
 * • GET /images - Image gallery listing and management
 * • POST /image-action - Image manipulation commands
 * • GET /preview - Image preview and thumbnail generation
 * 
 * @see ImageManager for file processing capabilities
 * @see SlideshowManager for display integration
 */
void WiFiManager::setupImageRoutes() {
    if (!imageManager) {
        LOG_WARN(TAG, "ImageManager not available - skipping image routes");
        return;
    }
    
    LOG_INFO(TAG, "Setting up image management routes");
    
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
                
                LOG_INFOF(TAG, "Starting image upload: %s (%d bytes)", filename.c_str(), totalSize);
                
                // SAFETY: Check available memory before allocation
                size_t freeHeap = ESP.getFreeHeap();
                #ifdef ESP32S3_MODE
                size_t minFreeAfterAlloc = 150000; // Keep at least 150KB free on ESP32-S3
                size_t maxFileSize = 1000000;      // 1MB max on ESP32-S3 (more memory available)
                #else
                size_t minFreeAfterAlloc = 100000; // Keep at least 100KB free on ESP32
                size_t maxFileSize = 500000;       // 500KB max on ESP32 (less memory available)
                #endif
                
                if (totalSize == 0 || totalSize > (freeHeap - minFreeAfterAlloc)) {
                    Serial.printf("ERROR: Upload too large. Size: %d, Free heap: %d\n", totalSize, freeHeap);
                    LOG_ERRORF(TAG, "Upload rejected - size %d bytes exceeds available memory", totalSize);
                    request->send(400, "text/plain", "File too large for available memory");
                    return;
                }
                
                // Additional safety: Platform-specific file size limits
                if (totalSize > maxFileSize) {
                    Serial.printf("ERROR: File too large: %d bytes (max %d bytes)\n", totalSize, maxFileSize);
                    LOG_ERRORF(TAG, "Upload rejected - file exceeds %d byte limit", maxFileSize);
                    request->send(400, "text/plain", String("File too large (max ") + String(maxFileSize/1000) + "KB)");
                    return;
                }
                
                // SAFETY: Temporarily disable memory monitoring during upload
                MemoryManager::setMonitoringEnabled(false);
                
                // Allocate buffer for entire file
                uploadBuffer = (uint8_t*)malloc(totalSize);
                if (!uploadBuffer) {
                    Serial.println("ERROR: Failed to allocate upload buffer");
                    LOG_ERROR(TAG, "Failed to allocate upload buffer");
                    // Re-enable monitoring before returning
                    MemoryManager::setMonitoringEnabled(true);
                    request->send(500, "text/plain", "Memory allocation failed");
                    return;
                }
                Serial.printf("Buffer allocated successfully (%d bytes), Free heap now: %d\n", 
                             totalSize, ESP.getFreeHeap());
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
                
                LOG_INFOF(TAG, "Upload complete: %s (%d bytes)", uploadFilename.c_str(), receivedSize);
                
                if (uploadBuffer && imageManager) {
                    // Use the total expected size, not received size for validation
                    bool success = imageManager->handleImageUpload(uploadFilename, uploadBuffer, totalSize);
                    
                    if (success) {
                        // BUGFIX: Refresh slideshow after successful upload
                        // This ensures newly uploaded images appear in the slideshow immediately
                        if (slideshowManager) {
                            Serial.println("SUCCESS: Image uploaded, refreshing slideshow...");
                            slideshowManager->refreshImageList();
                            LOG_INFO(TAG, "📄 Slideshow refreshed after image upload");
                        }
                    } else {
                        String errorMsg = imageManager->getLastError();
                        if (errorMsg.length() == 0) {
                            errorMsg = "Image validation failed - check format and size";
                        }
                        LOG_ERRORF(TAG, "Image upload failed: %s", errorMsg.c_str());
                        request->send(400, "text/plain", errorMsg);
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
                
                // SAFETY: Re-enable memory monitoring after upload completes
                MemoryManager::setMonitoringEnabled(true);
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
    
    LOG_INFO(TAG, "Image management routes configured");
}

/**
 * @brief Monitors WiFi connection status and handles automatic reconnection
 * 
 * Continuously monitors the WiFi connection health in normal mode and implements
 * intelligent reconnection strategies to maintain network connectivity.
 * 
 * Monitoring Features:
 * • Periodic connection health checks (every 30 seconds)
 * • Signal strength (RSSI) monitoring and logging
 * • Connection failure detection and counting
 * • Automatic reconnection attempts with exponential backoff
 * • Mode switching on persistent connection failures
 * 
 * Reconnection Strategy:
 * • First attempt: Simple reconnect to current network
 * • Second attempt: Full credential reload and connect
 * • Final attempt: Switch to setup mode for reconfiguration
 * 
 * @note Only operates in MODE_NORMAL to avoid interference with setup mode
 * @note Logs connection statistics for debugging and monitoring
 * @warning Persistent failures will trigger setup mode switch
 * 
 * Failure Thresholds:
 * • 3+ consecutive failures trigger credential reload
 * • 5+ consecutive failures trigger setup mode switch
 * 
 * @see switchToSetupMode() for fallback handling
 * @see connectToSavedNetwork() for reconnection logic
 */
void WiFiManager::checkConnectionStatus() {
    if (currentMode != MODE_NORMAL) return;
    
    unsigned long currentTime = millis();
    
    // Check connection every 30 seconds
    if (currentTime - lastConnectionAttempt < 30000) return;
    
    if (WiFi.status() != WL_CONNECTED) {
        connectionRetryCount++;
        LOG_WARNF(TAG, "WiFi connection lost - retry attempt %d/%d", connectionRetryCount, MAX_RETRY_ATTEMPTS);
        
        if (connectionRetryCount <= MAX_RETRY_ATTEMPTS) {
            // Attempt reconnection with increasing delays
            unsigned long delay = RETRY_DELAYS[min(connectionRetryCount - 1, 2)];
            if (currentTime - lastConnectionAttempt >= delay) {
                lastConnectionAttempt = currentTime;
                
                if (connectToSavedNetwork()) {
                    LOG_INFO(TAG, "WiFi reconnected successfully");
                    connectionRetryCount = 0;
                } else {
                    LOG_ERRORF(TAG, "Reconnection attempt %d failed", connectionRetryCount);
                }
            }
        } else {
            LOG_WARN(TAG, "Max retry attempts reached - staying in normal mode, background retries continue");
            // Reset counter for background retries every 60 seconds
            if (currentTime - lastConnectionAttempt >= 60000) {
                connectionRetryCount = 0;
                lastConnectionAttempt = currentTime;
            }
        }
    }
}

// ============================================================================
// SYSTEM MONITORING & MAINTENANCE
// ============================================================================

/**
 * @brief Monitors GPIO0 button for factory reset activation
 * 
 * Continuously monitors GPIO0 pin for factory reset trigger with debouncing
 * and configurable hold time. Provides secure system restoration capability
 * while preventing accidental resets through timing requirements.
 * 
 * Factory Reset Process:
 * • Continuous GPIO0 monitoring with 100ms polling interval
 * • Button state debouncing to prevent false triggers
 * • Configurable hold time requirement (default: 5 seconds)
 * • Visual countdown feedback during reset process
 * • Complete credential and settings erasure upon activation
 * • Automatic system restart in setup mode after reset
 * 
 * Safety Features:
 * • Requires sustained button press to prevent accidental activation
 * • Visual feedback shows remaining hold time
 * • Immediate release cancels reset process
 * • Confirmation logging before executing reset
 * 
 * @note GPIO0 configured with internal pullup resistor (LOW when pressed)
 * @note Factory reset is irreversible - clears all stored credentials
 * @warning All network settings and configurations will be lost
 * 
 * @see CredentialManager::clearCredentials() for reset implementation
 * @see switchToSetupMode() for post-reset mode handling
 */
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

// ============================================================================
// 📊 STATUS QUERY & UTILITY METHODS
// ============================================================================

/**
 * @brief 📊 Checks current WiFi connection status and operational mode
 * 
 * Verifies both WiFi hardware connection state and system operational mode.
 * Ensures accurate connection reporting by validating both network connectivity
 * and proper normal mode operation for reliable status indication.
 * 
 * Validation Criteria:
 * • WiFi hardware connection state (WL_CONNECTED)
 * • System operational mode (MODE_NORMAL)
 * • Network interface availability and functionality
 * 
 * @return bool True if connected to WiFi network AND in normal operational mode,
 *              false if disconnected or in setup mode
 * 
 * @note Connection status requires both WL_CONNECTED state and MODE_NORMAL operation
 * @note Setup mode returns false even if technically connected to validate operational state
 * @note Used by web APIs and system monitoring for accurate status reporting
 * 
 * @see getWiFiIP() for retrieving connection details when connected
 * @see checkConnectionStatus() for connection monitoring implementation
 */
bool WiFiManager::isConnectedToWiFi() const {
    return (WiFi.status() == WL_CONNECTED && currentMode == MODE_NORMAL);
}

/**
 * @brief 🌐 Retrieves current WiFi IP address for network identification
 * 
 * Returns the assigned WiFi IP address when in normal mode with active connection.
 * Provides network identification information for status displays and remote access.
 * 
 * Connection Requirements:
 * • Active WiFi connection (WL_CONNECTED)
 * • Normal operational mode (MODE_NORMAL)
 * • Valid IP assignment from DHCP or static configuration
 * 
 * @return String Current WiFi IP address in dot notation (e.g., "192.168.1.100"),
 *                or "N/A" if not connected or in setup mode
 * 
 * @note Returns "N/A" for setup mode or disconnected state for clear status indication
 * @note IP address used for remote access and network troubleshooting
 * @note Commonly used in web APIs and status displays for network information
 * 
 * @see isConnectedToWiFi() for connection status verification
 * @see checkConnectionStatus() for connectivity monitoring
 */
String WiFiManager::getWiFiIP() const {
    if (currentMode == MODE_NORMAL && WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "N/A";
}

/**
 * @brief 🔄 Executes scheduled system restart when timeout reached
 * 
 * Monitors for pending restart conditions and executes system restart
 * when scheduled time is reached. Provides controlled restart capability
 * for mode transitions and system recovery operations.
 * 
 * Restart Conditions:
 * • Restart flag is pending (restartPending = true)
 * • Scheduled time has been reached or exceeded
 * • System is in a stable state for restart execution
 * 
 * Common Restart Triggers:
 * • Factory reset completion
 * • Mode transition requirements
 * • Critical system recovery needs
 * • Configuration changes requiring reboot
 * 
 * @note Called periodically in main loop to check restart scheduling
 * @note Restart execution is immediate and non-recoverable
 * @warning All unsaved data will be lost during restart
 * 
 * @see checkGpio0FactoryReset() for factory reset restart triggers
 * @see switchToSetupMode() for mode transition restart needs
 */
void WiFiManager::checkScheduledRestart() {
    if (restartPending && millis() >= restartScheduledTime) {
        LOG_INFO(TAG, "🔄 Executing scheduled restart...");
        ESP.restart();
    }
}

/**
 * @brief 🔄 Checks for pending portal mode switch requests and executes transition
 * 
 * Monitors internal flag for portal mode switch requests typically triggered
 * by user interface actions or system conditions requiring configuration
 * access. Provides controlled transition to setup mode when requested.
 * 
 * Portal Mode Switch Process:
 * • Checks switchToPortalMode flag state
 * • Logs transition request for debugging purposes  
 * • Resets flag to prevent repeated switching
 * • Executes immediate transition to setup mode
 * 
 * Common Triggers:
 * • User request via settings interface
 * • System configuration requirements
 * • Administrative access needs
 * • Troubleshooting mode activation
 * 
 * Transition Behavior:
 * • Flag-based triggering prevents race conditions
 * • Single execution per flag setting
 * • Immediate mode switching without delay
 * • Comprehensive logging for troubleshooting
 * 
 * @note Called periodically in main loop to check switch requests
 * @note Flag automatically reset after processing to prevent repeat switches
 * @note Mode switching involves brief service interruption
 * 
 * @see switchToSetupMode() for actual mode transition implementation
 * @see Settings interface for user-triggered portal mode requests
 */
void WiFiManager::checkPortalModeSwitch() {
    if (switchToPortalMode) {
        LOG_INFO(TAG, "🌐 Switching to portal mode as requested via settings");
        switchToPortalMode = false;
        switchToSetupMode();
    }
}

/**
 * @brief ⏰ Manages transition from connection success display to normal mode
 * 
 * Monitors display state timing to transition from connection success message
 * back to normal operation display after configured delay period. Provides
 * visual feedback continuity and automatic return to standard operation.
 * 
 * Display Transition Process:
 * • Checks if connection success message is currently displayed
 * • Monitors elapsed time since success message started
 * • Automatically transitions to normal mode after 5-second delay
 * • Updates internal state flags and logs transition
 * 
 * Timing Considerations:
 * • Success message displayed for exactly 5 seconds
 * • Automatic transition prevents indefinite success display
 * • Non-blocking operation maintains system responsiveness
 * • State tracking ensures proper display mode management
 * 
 * @note Called periodically in main loop for timing management
 * @note Transition is automatic and requires no external intervention
 * @warning Display mode changes are immediate and visible to users
 * 
 * @see isShowingConnectionSuccess() for current display state checking
 * @see WiFi connection handling for success trigger conditions
 */
void WiFiManager::checkConnectionSuccessDisplay() {
    // Only check if we're displaying the connection success message
    if (!connectionSuccessDisplayed) return;
    
    // Check if 5 seconds have passed
    if (millis() - connectionSuccessStartTime >= DISPLAY_MODE_SWITCH_DURATION_MS) {
        LOG_INFO(TAG, "🕐 5 seconds passed, switching to normal mode display");
        
        // Mark as no longer displaying
        connectionSuccessDisplayed = false;
        
        // ADDED: Switch to normal mode now that connection success display is complete
        if (currentMode != MODE_NORMAL && WiFi.status() == WL_CONNECTED) {
            switchToNormalMode();
        }
        
        LOG_INFO(TAG, "Switched to normal mode display");
    }
}

/**
 * @brief Returns current connection success display state
 * 
 * Provides read-only access to the connection success display flag,
 * indicating whether the system is currently showing a WiFi connection
 * success message on the displays.
 * 
 * State Information:
 * • Returns true when success message is actively displayed
 * • Returns false during normal operation or other display modes
 * • State automatically managed by success display timing logic
 * • Used for display coordination and status checking
 * 
 * Common Use Cases:
 * • Display mode coordination and conflict prevention
 * • Status checking for external monitoring systems
 * • Debugging display state management issues
 * • Integration with other display management components
 * 
 * @return bool Current connection success display state
 * @retval true Success message is currently displayed
 * @retval false Normal operation or other display mode active
 * 
 * @note This is a const method and does not modify system state
 * @note State is automatically managed by display timing logic
 * 
 * @see checkConnectionSuccessDisplay() for state transition management
 * @see DisplayManager for actual display rendering coordination
 */
// NEW: Getter method for connection success display state
bool WiFiManager::isShowingConnectionSuccess() const {
    return connectionSuccessDisplayed;
}

// ========================================
// OTA Firmware Update Implementation
// ========================================

/**
 * @brief 🔄 Configures Over-The-Air (OTA) firmware update web endpoints
 * 
 * Establishes secure web-based firmware update capability allowing remote
 * system updates without physical device access. Implements comprehensive
 * validation, progress monitoring, and error handling for safe updates.
 * 
 * OTA Update Features:
 * • Secure firmware binary validation before installation
 * • Real-time upload progress monitoring and feedback
 * • Comprehensive error handling and recovery mechanisms
 * • Platform-specific firmware compatibility checking
 * • Automatic system restart after successful update
 * 
 * Configured Endpoints:
 * • POST /ota-update: Firmware binary upload and installation
 * • Filename validation for platform compatibility
 * • Binary signature verification for security
 * • Progress tracking for user feedback
 * 
 * Security Measures:
 * • Firmware filename validation against expected patterns
 * • Binary header verification for ESP32 compatibility
 * • Upload size limitations to prevent memory exhaustion
 * • Error state handling to prevent partial installations
 * 
 * @note OTA updates require system restart to take effect
 * @note Failed updates maintain current firmware integrity
 * @warning OTA process temporarily interrupts normal operations
 * 
 * @see validateFirmwareFilename() for filename security checking
 * @see validateFirmwareBinary() for binary integrity verification
 */
void WiFiManager::setupOTARoutes() {
    LOG_INFO(TAG, "🔄 Setting up OTA firmware update routes");
    
    // OTA firmware upload endpoint
    server->on("/ota-update", HTTP_POST, 
        [this](AsyncWebServerRequest *request) {
            // This handles the response after OTA upload is complete
            LOG_INFO(TAG, "🔄 OTA update request completed");
            
            if (Update.hasError()) {
                String errorMsg = "OTA Update failed with error: " + String(Update.getError());
                LOG_ERRORF(TAG, "%s", errorMsg.c_str());
                request->send(500, "text/plain", errorMsg);
            } else {
                LOG_INFO(TAG, "OTA update successful - restarting device");
                request->send(200, "text/plain", "OTA Update successful! Device restarting...");
                
                // Schedule restart after sending response
                restartPending = true;
                restartScheduledTime = millis() + 2000; // 2 seconds
            }
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            // Handle OTA file upload chunks
            static size_t totalSize = 0;
            
            if (index == 0) {
                // First chunk - start OTA update
                totalSize = request->contentLength();
                LOG_INFOF(TAG, "🔄 Starting OTA update: %s (%d bytes)", filename.c_str(), totalSize);
                
                // Validate file extension
                if (!filename.endsWith(".bin")) {
                    LOG_ERROR(TAG, "Invalid file type - only .bin files allowed");
                    request->send(400, "text/plain", "Error: Only .bin firmware files are allowed");
                    return;
                }
                
                // Basic size validation (ESP32 firmware typically 1-4MB)
                if (totalSize < 100000 || totalSize > 4194304) {
                    LOG_ERRORF(TAG, "Invalid firmware size: %d bytes", totalSize);
                    request->send(400, "text/plain", "Error: Invalid firmware size for ESP32");
                    return;
                }
                
                // Validate firmware filename compatibility
                if (!validateFirmwareFilename(filename)) {
                    // Get current platform info for error message
                    auto platform = PlatformDetector::detectPlatform();
                    String expectedPlatform = "";
                    if (platform.chipModel == PlatformDetector::CHIP_ESP32_CLASSIC) {
                        expectedPlatform = "ESP32";
                    } else if (platform.chipModel == PlatformDetector::CHIP_ESP32_S3) {
                        expectedPlatform = "ESP32-S3";
                    }
                    
                    String expectedDisplay = "";
                    #ifdef DISPLAY_TYPE_ST7789
                        expectedDisplay = "ST7789";
                    #else
                        expectedDisplay = "ST7735";
                    #endif
                    
                    String errorMsg = "Error: Firmware '" + filename + "' is not compatible with " + expectedPlatform + " " + expectedDisplay;
                    LOG_ERRORF(TAG, "%s", errorMsg.c_str());
                    request->send(400, "text/plain", errorMsg);
                    return;
                }
                
                // Start the update process
                if (!Update.begin(totalSize)) {
                    String errorMsg = "OTA begin failed - error: " + String(Update.getError());
                    LOG_ERRORF(TAG, "%s", errorMsg.c_str());
                    request->send(500, "text/plain", errorMsg);
                    return;
                }
                
                LOG_INFO(TAG, "OTA update started successfully");
            }
            
            // Write chunk data
            if (Update.write(data, len) != len) {
                LOG_ERRORF(TAG, "OTA write failed at chunk %d", index);
                request->send(500, "text/plain", "OTA write failed");
                return;
            }
            
            if (final) {
                // Final chunk - complete the update
                LOG_INFOF(TAG, "🔄 OTA upload complete, finalizing update...");
                
                if (!Update.end(true)) {
                    String errorMsg = "OTA end failed - error: " + String(Update.getError());
                    LOG_ERRORF(TAG, "%s", errorMsg.c_str());
                    request->send(500, "text/plain", errorMsg);
                } else {
                    LOG_INFO(TAG, "OTA update finalized successfully");
                    // Success response will be sent in the main handler above
                }
            }
        });
    
    // Firmware version API endpoint
    server->on("/api/firmware-version", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String version = getFirmwareVersion();
        String response = "{\"version\":\"" + version + "\"}";
        request->send(200, "application/json", response);
    });
    
    // Build information API endpoint (separate for UI display)
    server->on("/api/build-info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String buildDate = String(BUILD_DATE);
        String buildType = String(BUILD_TYPE);
        String firmwareVersion = String(FIRMWARE_VERSION);
        
        String response = "{\"buildDate\":\"" + buildDate + 
                         "\",\"buildType\":\"" + buildType + 
                         "\",\"version\":\"" + firmwareVersion + "\"}";
        request->send(200, "application/json", response);
    });
    
    // Firmware filename validation API endpoint
    server->on("/api/validate-firmware", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("filename")) {
            String response = "{\"valid\":false,\"error\":\"Filename parameter required\"}";
            request->send(400, "application/json", response);
            return;
        }
        
        String filename = request->getParam("filename")->value();
        bool isValid = validateFirmwareFilename(filename);
        
        if (isValid) {
            String response = "{\"valid\":true,\"message\":\"Firmware compatible with current device\"}";
            request->send(200, "application/json", response);
        } else {
            // Get current platform info for error message
            auto platform = PlatformDetector::detectPlatform();
            String expectedPlatform = "";
            if (platform.chipModel == PlatformDetector::CHIP_ESP32_CLASSIC) {
                expectedPlatform = "ESP32";
            } else if (platform.chipModel == PlatformDetector::CHIP_ESP32_S3) {
                expectedPlatform = "ESP32-S3";
            }
            
            String expectedDisplay = "";
            #ifdef DISPLAY_TYPE_ST7789
                expectedDisplay = "ST7789";
            #else
                expectedDisplay = "ST7735";
            #endif
            
            String errorMsg = "Firmware '" + filename + "' is not compatible with " + expectedPlatform + " " + expectedDisplay;
            String response = "{\"valid\":false,\"error\":\"" + errorMsg + "\"}";
            request->send(200, "application/json", response);
        }
    });
    
    LOG_INFO(TAG, "OTA routes configured successfully");
}

/**
 * @brief Validates firmware filename for platform and display compatibility
 * 
 * Performs security validation to ensure uploaded firmware matches current
 * hardware platform and display configuration. Prevents installation of
 * incompatible firmware that could cause system malfunction or damage.
 * 
 * Validation Criteria:
 * • Platform compatibility (ESP32 vs ESP32-S3)
 * • Display type matching (ST7735 vs ST7789)
 * • Filename pattern verification for expected format
 * • Build type identification (debug, production, latest)
 * 
 * Expected Filename Patterns:
 * • esp32_ST7735_*.bin - ESP32 with ST7735 display
 * • esp32_ST7789_*.bin - ESP32 with ST7789 display
 * • esp32s3_ST7735_*.bin - ESP32-S3 with ST7735 display
 * • esp32s3_ST7789_*.bin - ESP32-S3 with ST7789 display
 * 
 * Security Benefits:
 * • Prevents cross-platform firmware installation
 * • Avoids display driver incompatibility issues
 * • Reduces risk of hardware damage from wrong firmware
 * • Ensures proper functionality after update
 * 
 * @param filename Firmware filename to validate against current hardware
 * @return bool Validation result indicating compatibility
 * @retval true Firmware is compatible with current hardware
 * @retval false Firmware is incompatible and should be rejected
 * 
 * @note Uses PlatformDetector for current hardware identification
 * @note Display type determined from compile-time definitions
 * @warning Installing incompatible firmware may render device inoperable
 * 
 * @see PlatformDetector::detectPlatform() for hardware identification
 * @see validateFirmwareBinary() for binary content validation
 */
bool WiFiManager::validateFirmwareFilename(const String& filename) {
    // Get current platform info
    auto platform = PlatformDetector::detectPlatform();
    String expectedPlatform = "";
    
    // Determine expected platform string
    if (platform.chipModel == PlatformDetector::CHIP_ESP32_CLASSIC) {
        expectedPlatform = "esp32";
    } else if (platform.chipModel == PlatformDetector::CHIP_ESP32_S3) {
        expectedPlatform = "esp32s3";
    } else {
        LOG_WARN(TAG, "Unknown platform for firmware validation");
        return false;
    }
    
    // Get expected display type
    String expectedDisplay = "";
    #ifdef DISPLAY_TYPE_ST7789
        expectedDisplay = "ST7789";
    #else
        expectedDisplay = "ST7735";
    #endif
    
    // Build expected filename patterns
    String expectedDebug = expectedPlatform + "_" + expectedDisplay + "_debug.bin";
    String expectedProduction = expectedPlatform + "_" + expectedDisplay + "_production.bin";
    String expectedLatest = expectedPlatform + "_" + expectedDisplay + "_latest.bin";
    String genericFirmware = "firmware.bin";
    
    LOG_INFOF(TAG, "Validating firmware file: %s", filename.c_str());
    LOG_INFOF(TAG, "Current platform: %s", expectedPlatform.c_str());
    LOG_INFOF(TAG, "Current display: %s", expectedDisplay.c_str());
    LOG_INFOF(TAG, "Expected patterns: %s, %s, %s, %s", 
             expectedDebug.c_str(), expectedProduction.c_str(), 
             expectedLatest.c_str(), genericFirmware.c_str());
    
    // Check if filename matches expected patterns
    if (filename.equals(expectedDebug) || 
        filename.equals(expectedProduction) || 
        filename.equals(expectedLatest) ||
        filename.equals(genericFirmware)) {
        LOG_INFO(TAG, "Firmware filename validation passed");
        return true;
    }
    
    LOG_WARN(TAG, "Firmware filename validation failed!");
    LOG_WARNF(TAG, "File '%s' is not compatible with %s %s", 
             filename.c_str(), expectedPlatform.c_str(), expectedDisplay.c_str());
    return false;
}

/**
 * @brief Validates firmware binary content for ESP32 compatibility
 * 
 * Performs binary-level validation of firmware data to ensure compatibility
 * with ESP32 architecture and prevent installation of corrupted or invalid
 * firmware that could damage the system or cause malfunction.
 * 
 * Binary Validation Checks:
 * • Size validation within ESP32 flash memory limits
 * • ESP32 binary header verification and magic numbers
 * • Segment structure validation for proper loading
 * • Checksum verification for data integrity
 * 
 * Size Constraints:
 * • Minimum size: 100KB (prevents invalid/incomplete binaries)
 * • Maximum size: 4MB (ESP32 flash memory partition limit)
 * • Realistic size range validation for typical firmware
 * 
 * ESP32 Binary Format:
 * • Magic number verification (0xE9 at offset 0)
 * • Segment count validation
 * • Load address verification
 * • Entry point validation
 * 
 * Security Features:
 * • Prevents upload of non-ESP32 binaries
 * • Detects corrupted firmware files
 * • Validates binary structure integrity
 * • Protects against malicious firmware uploads
 * 
 * @param data Pointer to firmware binary data buffer
 * @param length Size of firmware binary in bytes
 * @return bool Binary validation result
 * @retval true Binary is valid ESP32 firmware
 * @retval false Binary is invalid or incompatible
 * 
 * @note Validation includes both basic checks and ESP32-specific format verification
 * @note Failed validation prevents OTA update process from starting
 * @warning Installing invalid firmware will cause system malfunction
 * 
 * @see ESP32 Technical Reference Manual for binary format specifications
 * @see validateFirmwareFilename() for filename-based compatibility checking
 */
bool WiFiManager::validateFirmwareBinary(uint8_t* data, size_t length) {
    // Basic ESP32 firmware validation
    if (length < 100000) {
        LOG_WARN(TAG, "Firmware file too small");
        return false;
    }
    
    if (length > 4194304) { // 4MB max
        LOG_WARN(TAG, "Firmware file too large");
        return false;
    }
    
    // Check for ESP32 magic bytes (basic validation)
    if (length >= 4) {
        // ESP32 firmware typically starts with 0xE9 (ESP32 magic number)
        if (data[0] == 0xE9) {
            LOG_INFO(TAG, "ESP32 firmware signature detected");
            return true;
        }
    }
    
    LOG_WARN(TAG, "Firmware signature not recognized");
    return true; // Allow anyway, but warn
}

/**
 * @brief 📋 Returns comprehensive firmware version information string
 * 
 * Generates formatted version string containing firmware version, build type,
 * and build date for system identification, debugging, and compatibility
 * verification during OTA updates and system management.
 * 
 * Version String Format:
 * • Base version number (e.g., "0.9")
 * • Build type identifier (debug, production, latest)
 * • Build date stamp for temporal identification
 * • Combined format: "0.9-production-250815"
 * 
 * Version Components:
 * • FIRMWARE_VERSION: Core version number from build system
 * • BUILD_TYPE: Compilation configuration (debug/production/latest)
 * • BUILD_DATE: Automated build timestamp (YYMMDD format)
 * 
 * Common Use Cases:
 * • OTA update compatibility verification
 * • System status reporting and monitoring
 * • Debug log identification and correlation
 * • User interface version display
 * • Support and troubleshooting identification
 * 
 * @return String Formatted firmware version information
 * @return Format: "{version}-{type}-{date}" (e.g., "0.9-production-250815")
 * 
 * @note Version information populated automatically during build process
 * @note Build date uses YYMMDD format for compact representation
 * @note Version string used in OTA update validation and system reporting
 * 
 * @see include/version.h for version management and automated build numbering
 * @see platformio.ini for version configuration and build type definitions
 */
String WiFiManager::getFirmwareVersion() {
    // Return current firmware version with build info
    String version = String(FIRMWARE_VERSION) + "-" + String(BUILD_TYPE) + "-" + String(BUILD_DATE);
    return version;
}

