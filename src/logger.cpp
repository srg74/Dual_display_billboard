#include "logger.h"
#include <WiFi.h>
#include <stdarg.h>

bool Logger::initialized = false;
static Logger::Level currentLevel = Logger::INFO;

void Logger::ensureInitialized() {
    if (!initialized) {
        init();
    }
}

void Logger::init(unsigned long baudRate) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED
    Serial.begin(baudRate);
    while (!Serial && millis() < 3000) {
        yield(); // Non-blocking yield instead of delay
    }
    initialized = true;
    
    Serial.println();
    Serial.println("=================================");
    Serial.println("🚀 Billboard System Logger v1.0");
    Serial.println("=================================");
    #endif
}

void Logger::setLevel(Level level) {
    currentLevel = level;
}

Logger::Level Logger::getLevel() {
    return currentLevel;
}

void Logger::log(Level level, const String& tag, const String& message) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED
    ensureInitialized();
    
    // Check if this level should be logged based on build flags
    bool shouldLog = false;
    switch (level) {
        case ERROR:
            shouldLog = LOGGER_LEVEL_ERROR;
            break;
        case WARN:
            shouldLog = LOGGER_LEVEL_WARN;
            break;
        case INFO:
            shouldLog = LOGGER_LEVEL_INFO;
            break;
        case DEBUG:
            shouldLog = LOGGER_LEVEL_DEBUG;
            break;
        case VERBOSE:
            shouldLog = LOGGER_LEVEL_VERBOSE;
            break;
    }
    
    if (!shouldLog || level > currentLevel) return;
    
    // Get timestamp
    unsigned long timestamp = millis();
    
    // Level prefix
    String prefix;
    switch (level) {
        case ERROR:   prefix = "❌ [ERROR]  "; break;
        case WARN:    prefix = "⚠️  [WARN]   "; break;
        case INFO:    prefix = "ℹ️  [INFO]   "; break;
        case DEBUG:   prefix = "🔧 [DEBUG]  "; break;
        case VERBOSE: prefix = "🔍 [VERBOSE]"; break;
    }
    
    // Format: [timestamp] 🔧 [DEBUG] [TAG] Message
    Serial.printf("[%08lu] %s [%s] %s\n", 
                  timestamp, 
                  prefix.c_str(), 
                  tag.c_str(), 
                  message.c_str());
    #endif
}

void Logger::logf(Level level, const String& tag, const char* format, ...) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log(level, tag, String(buffer));
    #endif
}

// Convenience functions
void Logger::error(const String& tag, const String& message) {
    log(ERROR, tag, message);
}

void Logger::warn(const String& tag, const String& message) {
    log(WARN, tag, message);
}

void Logger::info(const String& tag, const String& message) {
    log(INFO, tag, message);
}

void Logger::debug(const String& tag, const String& message) {
    log(DEBUG, tag, message);
}

void Logger::verbose(const String& tag, const String& message) {
    log(VERBOSE, tag, message);
}

// Printf-style functions
void Logger::errorf(const String& tag, const char* format, ...) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_ERROR
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    error(tag, String(buffer));
    #endif
}

void Logger::warnf(const String& tag, const char* format, ...) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_WARN
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    warn(tag, String(buffer));
    #endif
}

void Logger::infof(const String& tag, const char* format, ...) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_INFO
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    info(tag, String(buffer));
    #endif
}

void Logger::debugf(const String& tag, const char* format, ...) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_DEBUG
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    debug(tag, String(buffer));
    #endif
}

void Logger::verbosef(const String& tag, const char* format, ...) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_VERBOSE
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    verbose(tag, String(buffer));
    #endif
}

void Logger::printSystemInfo() {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_INFO
    info("SYSTEM", "=== System Information ===");
    infof("SYSTEM", "Chip Model: %s", ESP.getChipModel());
    infof("SYSTEM", "Chip Revision: %d", ESP.getChipRevision());
    infof("SYSTEM", "CPU Freq: %d MHz", ESP.getCpuFreqMHz());
    infof("SYSTEM", "Flash Size: %d bytes", ESP.getFlashChipSize());
    infof("SYSTEM", "Free Heap: %d bytes", ESP.getFreeHeap());
    infof("SYSTEM", "SDK Version: %s", ESP.getSdkVersion());
    #endif
}

void Logger::printWiFiStatus() {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_INFO
    info("WIFI", "=== WiFi Status ===");
    infof("WIFI", "Mode: %s", WiFi.getMode() == WIFI_AP ? "AP" : 
                             WiFi.getMode() == WIFI_STA ? "STA" : 
                             WiFi.getMode() == WIFI_AP_STA ? "AP+STA" : "OFF");
    
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        infof("WIFI", "AP SSID: %s", WiFi.softAPSSID().c_str());
        infof("WIFI", "AP IP: %s", WiFi.softAPIP().toString().c_str());
        infof("WIFI", "AP Clients: %d", WiFi.softAPgetStationNum());
    }
    
    if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) {
        if (WiFi.isConnected()) {
            infof("WIFI", "Connected to: %s", WiFi.SSID().c_str());
            infof("WIFI", "STA IP: %s", WiFi.localIP().toString().c_str());
            infof("WIFI", "Signal: %d dBm", WiFi.RSSI());
        } else {
            warn("WIFI", "Not connected to any network");
        }
    }
    #endif
}

void Logger::printMemoryInfo() {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED && LOGGER_LEVEL_DEBUG
    info("MEMORY", "=== Memory Information ===");
    infof("MEMORY", "Free Heap: %d bytes", ESP.getFreeHeap());
    infof("MEMORY", "Heap Size: %d bytes", ESP.getHeapSize());
    infof("MEMORY", "Min Free Heap: %d bytes", ESP.getMinFreeHeap());
    infof("MEMORY", "Max Alloc Heap: %d bytes", ESP.getMaxAllocHeap());
    
    #ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
        infof("MEMORY", "PSRAM Size: %d bytes", ESP.getPsramSize());
        infof("MEMORY", "Free PSRAM: %d bytes", ESP.getFreePsram());
    } else {
        warn("MEMORY", "PSRAM not found");
    }
    #endif
    #endif
}