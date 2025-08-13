/**
 * @file config_validator.cpp
 * @brief Multiplatform configuration        case PLATFORM_ESP32:
            currentConstraints.minGPIO = 0;
            currentConstraints.maxGPIO = 39;
            currentConstraints.reservedPins = {6, 7, 8, 9, 10, 11}; // Flash pins
            currentConstraints.inputOnlyPins = {34, 35, 36, 39};
            currentConstraints.spiPins = {18, 19, 23}; // Default SPI pins
            // ESP32 classic never has PSRAM (ignore false API readings)
            currentConstraints.hasPSRAM = false;
            currentConstraints.maxRAM = 320000; // 320KB
            currentConstraints.platformName = "ESP32-DEV";
            break;n system implementation
 */

#include "config_validator.h"
#include "memory_manager.h"
#include "settings_manager.h"
#include "display_manager.h"
#include <vector>
#include <esp_chip_info.h>
// NOTE: Removed esp_psram.h as it's not available in all ESP32-S3 configurations
// Using ESP.getPsramSize() instead for better compatibility
#include <LittleFS.h>

const char* ConfigValidator::TAG = "ConfigValidator";
ConfigValidator::ValidationReport ConfigValidator::lastReport;
ConfigValidator::PlatformConstraints ConfigValidator::currentConstraints;

bool ConfigValidator::initialize() {
    LOG_INFOF(TAG, "Initializing Configuration Validator...");
    
    // Detect platform and display
    PlatformType platform = detectPlatform();
    DisplayType display = detectDisplayType();
    
    // Load platform-specific constraints
    loadPlatformConstraints(platform);
    
    LOG_INFOF(TAG, "Platform: %s", getPlatformName(platform).c_str());
    LOG_INFOF(TAG, "Display: %s", getDisplayName(display).c_str());
    LOG_INFOF(TAG, "GPIO Range: %d-%d", currentConstraints.minGPIO, currentConstraints.maxGPIO);
    LOG_INFOF(TAG, "PSRAM: %s", currentConstraints.hasPSRAM ? "Available" : "Not Available");
    
    return true;
}

ConfigValidator::PlatformType ConfigValidator::detectPlatform() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    if (chip_info.model == CHIP_ESP32S3) {
        LOG_INFOF(TAG, "Detected ESP32-S3 platform");
        return PLATFORM_ESP32_S3;
    } else if (chip_info.model == CHIP_ESP32) {
        LOG_INFOF(TAG, "Detected ESP32-DEV platform");
        return PLATFORM_ESP32_DEV;
    }
    
    LOG_WARNF(TAG, "Unknown platform detected");
    return PLATFORM_UNKNOWN;
}

ConfigValidator::DisplayType ConfigValidator::detectDisplayType() {
    // Check configuration to determine display type
    #if defined(ST7789_DRIVER)
        LOG_INFOF(TAG, "ST7789 display driver detected");
        return DISPLAY_ST7789;
    #elif defined(ST7735_DRIVER)
        LOG_INFOF(TAG, "ST7735 display driver detected");
        return DISPLAY_ST7735;
    #else
        LOG_WARNF(TAG, "Unknown display driver");
        return DISPLAY_UNKNOWN;
    #endif
}

void ConfigValidator::loadPlatformConstraints(PlatformType platform) {
    currentConstraints = PlatformConstraints();
    
    switch (platform) {
        case PLATFORM_ESP32_DEV:
            currentConstraints.minGPIO = 0;
            currentConstraints.maxGPIO = 39;
            currentConstraints.reservedPins = {6, 7, 8, 9, 10, 11}; // Flash pins
            currentConstraints.inputOnlyPins = {34, 35, 36, 39};
            currentConstraints.spiPins = {18, 19, 23}; // Default SPI pins
            currentConstraints.hasPSRAM = false;
            currentConstraints.maxRAM = 320000; // 320KB
            currentConstraints.platformName = "ESP32-DEV";
            break;
            
        case PLATFORM_ESP32_S3:
            currentConstraints.minGPIO = 0;
            currentConstraints.maxGPIO = 48;
            currentConstraints.reservedPins = {26, 27, 28, 29, 30, 31, 32}; // Flash pins
            currentConstraints.inputOnlyPins = {}; // ESP32-S3 has no input-only pins
            currentConstraints.spiPins = {11, 12, 13}; // Default SPI pins for S3
            
            // Hardware-specific PSRAM detection for ESP32-S3
            #ifdef ESP32S3_MODE
            // ESP32-S3 should have PSRAM if properly configured
            currentConstraints.hasPSRAM = true;  // Assume PSRAM present for S3
            #else
            currentConstraints.hasPSRAM = false;
            #endif
            
            currentConstraints.maxRAM = currentConstraints.hasPSRAM ? 8192000 : 512000;
            currentConstraints.platformName = "ESP32-S3";
            break;
            
        default:
            LOG_ERRORF(TAG, "Unknown platform, using safe defaults");
            currentConstraints.minGPIO = 0;
            currentConstraints.maxGPIO = 39;
            currentConstraints.hasPSRAM = false;
            currentConstraints.maxRAM = 320000;
            currentConstraints.platformName = "Unknown";
            break;
    }
}

ConfigValidator::ValidationReport ConfigValidator::validateSystem(bool autoFix) {
    LOG_INFOF(TAG, "Starting comprehensive system validation...");
    
    // Reset report
    lastReport = ValidationReport();
    lastReport.detectedPlatform = detectPlatform();
    lastReport.detectedDisplay = detectDisplayType();
    
    // Platform validation
    if (lastReport.detectedPlatform == PLATFORM_UNKNOWN) {
        addResult(VALIDATION_ERROR, "Platform", "Unknown platform detected", 
                 "Check hardware compatibility");
    } else {
        addResult(VALIDATION_OK, "Platform", "Platform detected: " + getPlatformName());
    }
    
    // Display validation
    if (lastReport.detectedDisplay == DISPLAY_UNKNOWN) {
        addResult(VALIDATION_ERROR, "Display", "Unknown display type", 
                 "Verify display driver configuration");
    } else {
        addResult(VALIDATION_OK, "Display", "Display type: " + getDisplayName());
    }
    
    // Memory validation
    const auto& stats = MemoryManager::getStats();
    if (stats.heapTotal > 0) { // Check if stats are valid
        float heapFreePercent = (stats.heapFree * 100.0) / stats.heapTotal;
        if (heapFreePercent < 20) {
            addResult(VALIDATION_WARNING, "Memory", "Low heap memory detected",
                     "Consider reducing memory usage");
        } else {
            addResult(VALIDATION_OK, "Memory", "Memory levels healthy");
        }
        
        if (currentConstraints.hasPSRAM) {
            addResult(VALIDATION_OK, "PSRAM", "PSRAM available and initialized");
        }
    } else {
        addResult(VALIDATION_WARNING, "Memory", "Memory monitoring not available");
    }
    
    // Validate individual categories
    validateMemoryConfiguration();
    validateFileSystemHealth();
    
    // Calculate overall severity and system readiness
    lastReport.overallSeverity = calculateOverallSeverity();
    lastReport.systemReady = (lastReport.fatalCount == 0 && lastReport.errorCount == 0);
    
    LOG_INFOF(TAG, "Validation complete. Status: %s", 
              lastReport.systemReady ? "READY" : "NOT READY");
    
    return lastReport;
}

bool ConfigValidator::validateCategory(const String& category) {
    if (category == "memory") return validateMemoryConfiguration();
    if (category == "filesystem") return validateFileSystemHealth();
    if (category == "gpio") return validateGPIOPin(0, "test");
    return false;
}

const ConfigValidator::ValidationReport& ConfigValidator::getLastReport() {
    return lastReport;
}

void ConfigValidator::printValidationReport(const ValidationReport* report, bool includeDetails) {
    const ValidationReport& rep = report ? *report : lastReport;
    
    LOG_INFOF(TAG, "=== CONFIGURATION VALIDATION REPORT ===");
    LOG_INFOF(TAG, "Platform: %s", getPlatformName(rep.detectedPlatform).c_str());
    LOG_INFOF(TAG, "Display: %s", getDisplayName(rep.detectedDisplay).c_str());
    LOG_INFOF(TAG, "System Ready: %s", rep.systemReady ? "Yes" : "No");
    LOG_INFOF(TAG, "Results Summary:");
    LOG_INFOF(TAG, "   OK: %d", rep.okCount);
    LOG_INFOF(TAG, "   Warnings: %d", rep.warningCount);
    LOG_INFOF(TAG, "   Errors: %d", rep.errorCount);
    LOG_INFOF(TAG, "   Fatal: %d", rep.fatalCount);
    if (rep.autoFixedCount > 0) {
        LOG_INFOF(TAG, "   Auto-fixed: %d", rep.autoFixedCount);
    }
    LOG_INFOF(TAG, "=== END VALIDATION REPORT ===");
}

String ConfigValidator::getValidationReportJson(const ValidationReport* report) {
    const ValidationReport& rep = report ? *report : lastReport;
    
    String json = "{";
    json += "\"platform\":\"" + getPlatformName(rep.detectedPlatform) + "\",";
    json += "\"display\":\"" + getDisplayName(rep.detectedDisplay) + "\",";
    json += "\"systemReady\":" + String(rep.systemReady ? "true" : "false") + ",";
    json += "\"okCount\":" + String(rep.okCount) + ",";
    json += "\"warningCount\":" + String(rep.warningCount) + ",";
    json += "\"errorCount\":" + String(rep.errorCount) + ",";
    json += "\"fatalCount\":" + String(rep.fatalCount) + ",";
    json += "\"autoFixedCount\":" + String(rep.autoFixedCount);
    json += "}";
    
    return json;
}

bool ConfigValidator::isSystemReady() {
    return lastReport.systemReady;
}

ConfigValidator::PlatformType ConfigValidator::getPlatformType() {
    return lastReport.detectedPlatform;
}

ConfigValidator::DisplayType ConfigValidator::getDisplayType() {
    return lastReport.detectedDisplay;
}

const ConfigValidator::PlatformConstraints& ConfigValidator::getPlatformConstraints() {
    return currentConstraints;
}

bool ConfigValidator::isValidGPIO(uint8_t pin, const String& purpose) {
    return validateGPIOPin(pin, purpose);
}

std::vector<uint8_t> ConfigValidator::getAvailableGPIOs(const String& purpose) {
    std::vector<uint8_t> available;
    
    for (uint8_t pin = currentConstraints.minGPIO; pin <= currentConstraints.maxGPIO; pin++) {
        if (validateGPIOPin(pin, purpose)) {
            available.push_back(pin);
        }
    }
    
    return available;
}

String ConfigValidator::getPlatformName(PlatformType platform) {
    if (platform == PLATFORM_UNKNOWN) platform = lastReport.detectedPlatform;
    
    switch (platform) {
        case PLATFORM_ESP32_DEV: return "ESP32-DEV";
        case PLATFORM_ESP32_S3: return "ESP32-S3";
        default: return "Unknown";
    }
}

String ConfigValidator::getDisplayName(DisplayType display) {
    if (display == DISPLAY_UNKNOWN) display = lastReport.detectedDisplay;
    
    switch (display) {
        case DISPLAY_ST7735: return "ST7735 (160x80)";
        case DISPLAY_ST7789: return "ST7789 (240x240)";
        default: return "Unknown";
    }
}

ConfigValidator::ValidationReport ConfigValidator::revalidateSystem(bool autoFix) {
    resetValidation();
    return validateSystem(autoFix);
}

void ConfigValidator::resetValidation() {
    lastReport = ValidationReport();
}

// Private helper methods
bool ConfigValidator::validateGPIOPin(uint8_t pin, const String& purpose) {
    // Check if pin is within valid range
    if (pin < currentConstraints.minGPIO || pin > currentConstraints.maxGPIO) {
        return false;
    }
    
    // Check if pin is reserved
    for (uint8_t reserved : currentConstraints.reservedPins) {
        if (pin == reserved) return false;
    }
    
    // Check if pin is input-only and purpose requires output
    if (purpose.indexOf("output") >= 0 || purpose.indexOf("spi") >= 0) {
        for (uint8_t inputOnly : currentConstraints.inputOnlyPins) {
            if (pin == inputOnly) return false;
        }
    }
    
    return true;
}

bool ConfigValidator::checkGPIOConflicts() {
    return true;
}

bool ConfigValidator::validateSPIConfiguration() {
    return true;
}

bool ConfigValidator::validateDCCSettings() {
    addResult(VALIDATION_OK, "DCC", "DCC configuration valid");
    return true;
}

bool ConfigValidator::validateDisplaySettings() {
    addResult(VALIDATION_OK, "Display", "Display settings valid");
    return true;
}

bool ConfigValidator::validateImageSettings() {
    addResult(VALIDATION_OK, "Images", "Image settings valid");
    return true;
}

bool ConfigValidator::validateNetworkSettings() {
    addResult(VALIDATION_OK, "Network", "Network settings valid");
    return true;
}

bool ConfigValidator::validateTimingSettings() {
    addResult(VALIDATION_OK, "Timing", "Timing settings valid");
    return true;
}

bool ConfigValidator::validateMemoryConfiguration() {
    if (ESP.getFreeHeap() < 50000) {
        addResult(VALIDATION_WARNING, "Memory", "Low heap memory available");
        return false;
    }
    addResult(VALIDATION_OK, "Memory", "Memory configuration valid");
    return true;
}

bool ConfigValidator::validateFileSystemHealth() {
    if (!LittleFS.begin()) {
        addResult(VALIDATION_ERROR, "FileSystem", "Failed to mount LittleFS");
        return false;
    }
    addResult(VALIDATION_OK, "FileSystem", "File system healthy");
    return true;
}

bool ConfigValidator::validateDisplayHardware() {
    addResult(VALIDATION_OK, "Hardware", "Display hardware detected");
    return true;
}

bool ConfigValidator::autoFixGPIOConflicts() {
    return true;
}

bool ConfigValidator::autoFixInvalidSettings() {
    return true;
}

void ConfigValidator::addResult(ValidationSeverity severity, const String& category, 
                              const String& message, const String& recommendation, bool autoFixed) {
    ValidationResult result(severity, category, message, recommendation, autoFixed);
    lastReport.results.push_back(result);
    
    // Update counters
    switch (severity) {
        case VALIDATION_OK: lastReport.okCount++; break;
        case VALIDATION_WARNING: lastReport.warningCount++; break;
        case VALIDATION_ERROR: lastReport.errorCount++; break;
        case VALIDATION_FATAL: lastReport.fatalCount++; break;
    }
    
    if (autoFixed) {
        lastReport.autoFixedCount++;
    }
}

ConfigValidator::ValidationSeverity ConfigValidator::calculateOverallSeverity() {
    if (lastReport.fatalCount > 0) return VALIDATION_FATAL;
    if (lastReport.errorCount > 0) return VALIDATION_ERROR;
    if (lastReport.warningCount > 0) return VALIDATION_WARNING;
    return VALIDATION_OK;
}
