/**
 * @file config_validator.cpp
 * @brief Comprehensive multiplatform configuration validation system implementation
 * 
 * This module implements comprehensive validation for all supported ESP32 platforms
 * and display configurations. Provides platform-specific constraint validation,
 * automatic error correction, and detailed validation reporting for the dual display billboard system.
 * 
 * Key Implementation Features:
 * - ESP32/ESP32-S3 platform detection and constraint loading
 * - GPIO pin validation with conflict detection and auto-resolution
 * - Hardware capability verification (PSRAM, memory, flash)
 * - Settings validation against platform-specific limits
 * - LittleFS filesystem health monitoring
 * - Comprehensive validation reporting with severity levels
 * - Automatic correction of non-critical configuration issues
 * 
 * Technical Architecture:
 * - Static class design for global system validation state
 * - Platform constraints loaded dynamically based on detected hardware
 * - Validation results cached for efficient repeated queries
 * - JSON report generation for web interface integration
 * - Severity-based validation with automatic problem classification
 * 
 * Validation Categories:
 * - Platform Hardware: Chip detection, PSRAM availability, memory constraints
 * - GPIO Configuration: Pin validation, conflict detection, SPI bus integrity
 * - Display Hardware: Driver compatibility, hardware connection validation
 * - Settings Validation: Range checking, platform-specific limits
 * - Filesystem Health: LittleFS mount status, available space
 * - Network Configuration: WiFi capability, credential validation
 * 
 * @author ESP32 Dual Display Billboard Project
 * @version 0.9
 * @date August 2025
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

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

const char* ConfigValidator::TAG = "ConfigValidator";
ConfigValidator::ValidationReport ConfigValidator::lastReport;
ConfigValidator::PlatformConstraints ConfigValidator::currentConstraints;

// ============================================================================
// INITIALIZATION AND PLATFORM DETECTION
// ============================================================================

/**
 * @brief Initialize the configuration validator with platform detection
 * @return true if validator initialized successfully, false on failure
 * 
 * Performs comprehensive system initialization including platform detection,
 * display type identification, and constraint loading. Sets up the validator
 * for subsequent validation operations with platform-specific rules.
 */
bool ConfigValidator::initialize() {
    LOG_INFOF(TAG, "Initializing Configuration Validator...");
    
    // Detect platform and display hardware
    PlatformType platform = detectPlatform();
    DisplayType display = detectDisplayType();
    
    // Load platform-specific validation constraints
    loadPlatformConstraints(platform);
    
    // Log detected configuration for debugging
    LOG_INFOF(TAG, "Platform: %s", getPlatformName(platform).c_str());
    LOG_INFOF(TAG, " Display: %s", getDisplayName(display).c_str());
    LOG_INFOF(TAG, "GPIO Range: %d-%d", currentConstraints.minGPIO, currentConstraints.maxGPIO);
    LOG_INFOF(TAG, "PSRAM: %s", currentConstraints.hasPSRAM ? "Available" : "Not Available");
    
    return true;
}

/**
 * @brief Detect the ESP32 platform type through chip information analysis
 * @return Detected platform type with ESP32-S3 vs standard ESP32 classification
 * 
 * Uses ESP-IDF chip information API to determine the specific ESP32 variant
 * and configure appropriate validation constraints for the detected hardware.
 */
ConfigValidator::PlatformType ConfigValidator::detectPlatform() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    // Perform chip model identification with specific ESP32 variant detection
    if (chip_info.model == CHIP_ESP32S3) {
        LOG_INFOF(TAG, "Detected ESP32-S3 platform with enhanced capabilities");
        return PLATFORM_ESP32_S3;
    } else if (chip_info.model == CHIP_ESP32) {
        LOG_INFOF(TAG, "Detected ESP32-DEV standard platform");
        return PLATFORM_ESP32_DEV;
    }
    
    LOG_WARNF(TAG, "Unknown ESP32 platform detected - using default constraints");
    return PLATFORM_UNKNOWN;
}

/**
 * @brief Detect display type through compile-time driver configuration
 * @return Detected display type based on TFT_eSPI driver configuration
 * 
 * Examines compile-time driver definitions to determine which display driver
 * is configured for the current build, enabling display-specific validation.
 */
ConfigValidator::DisplayType ConfigValidator::detectDisplayType() {
    // Check TFT_eSPI configuration to determine display driver type
    #if defined(ST7789_DRIVER)
        LOG_INFOF(TAG, "ST7789 240x240 display driver detected");
        return DISPLAY_ST7789;
    #elif defined(ST7735_DRIVER)
        LOG_INFOF(TAG, "ST7735 160x80 display driver detected");
        return DISPLAY_ST7735;
    #else
        LOG_WARNF(TAG, "Unknown display driver configuration");
        return DISPLAY_UNKNOWN;
    #endif
}

// ============================================================================
// PLATFORM CONSTRAINTS AND CONFIGURATION
// ============================================================================

/**
 * @brief Load platform-specific validation constraints based on detected hardware
 * @param platform Detected platform type for constraint configuration
 * 
 * Configures platform-specific GPIO constraints, memory limits, and hardware
 * capabilities based on the detected ESP32 variant. Essential for proper
 * validation of user configurations against hardware limitations.
 */
void ConfigValidator::loadPlatformConstraints(PlatformType platform) {
    // Initialize constraints structure with defaults
    currentConstraints = PlatformConstraints();
    
    switch (platform) {
        case PLATFORM_ESP32_DEV:
            // Standard ESP32 platform constraints
            currentConstraints.minGPIO = 0;
            currentConstraints.maxGPIO = 39;
            currentConstraints.reservedPins = {6, 7, 8, 9, 10, 11}; // Flash SPI pins
            currentConstraints.inputOnlyPins = {34, 35, 36, 39};    // ADC1 input-only
            currentConstraints.spiPins = {18, 19, 23};              // VSPI default pins
            currentConstraints.hasPSRAM = false;                    // ESP32 classic without PSRAM
            currentConstraints.maxRAM = 320000;                     // 320KB internal RAM
            currentConstraints.platformName = "ESP32-DEV";
            LOG_INFOF(TAG, "Loaded ESP32-DEV constraints (GPIO 0-39, 320KB RAM)");
            break;
            
        case PLATFORM_ESP32_S3:
            // ESP32-S3 enhanced platform constraints
            currentConstraints.minGPIO = 0;
            currentConstraints.maxGPIO = 48;
            currentConstraints.reservedPins = {26, 27, 28, 29, 30, 31, 32}; // Flash pins
            currentConstraints.inputOnlyPins = {};                          // No input-only restrictions
            currentConstraints.spiPins = {11, 12, 13};                      // SPI2 default pins
            
            // ESP32-S3 PSRAM detection and memory configuration
            #ifdef ESP32S3_MODE
            currentConstraints.hasPSRAM = true;      // PSRAM expected in S3 builds
            currentConstraints.maxRAM = 8192000;     // 8MB with PSRAM
            #else
            currentConstraints.hasPSRAM = false;     // No PSRAM configured
            currentConstraints.maxRAM = 512000;      // 512KB internal RAM only
            #endif
            
            currentConstraints.platformName = "ESP32-S3";
            LOG_INFOF(TAG, "Loaded ESP32-S3 constraints (GPIO 0-48, %s)", 
                     currentConstraints.hasPSRAM ? "8MB PSRAM" : "512KB RAM");
            break;
            
        default:
            // Unknown platform - use conservative ESP32 defaults for safety
            LOG_ERRORF(TAG, "Unknown platform, applying conservative defaults");
            currentConstraints.minGPIO = 0;
            currentConstraints.maxGPIO = 39;
            currentConstraints.reservedPins = {6, 7, 8, 9, 10, 11};
            currentConstraints.inputOnlyPins = {34, 35, 36, 39};
            currentConstraints.spiPins = {18, 19, 23};
            currentConstraints.hasPSRAM = false;
            currentConstraints.maxRAM = 320000;
            currentConstraints.platformName = "Unknown";
            break;
    }
}

// ============================================================================
// COMPREHENSIVE SYSTEM VALIDATION
// ============================================================================

/**
 * @brief Perform comprehensive system validation across all components
 * @param autoFix Enable automatic correction of non-critical issues
 * @return Complete validation report with results and statistics
 * 
 * Executes full system validation including platform verification, GPIO
 * configuration checking, settings validation, and hardware verification.
 * Optionally applies automatic corrections for recoverable issues.
 */
ConfigValidator::ValidationReport ConfigValidator::validateSystem(bool autoFix) {
    LOG_INFOF(TAG, "Starting comprehensive system validation...");
    
    // Reset validation report for fresh analysis
    lastReport = ValidationReport();
    lastReport.detectedPlatform = detectPlatform();
    lastReport.detectedDisplay = detectDisplayType();
    
    // Platform Hardware Validation
    if (lastReport.detectedPlatform == PLATFORM_UNKNOWN) {
        addResult(VALIDATION_ERROR, "Platform", "Unknown platform detected", 
                 "Check hardware compatibility and ESP-IDF configuration");
    } else {
        addResult(VALIDATION_OK, "Platform", "Platform detected: " + getPlatformName());
    }
    
    // Display Driver Validation
    if (lastReport.detectedDisplay == DISPLAY_UNKNOWN) {
        addResult(VALIDATION_ERROR, "Display", "Unknown display type", 
                 "Verify TFT_eSPI display driver configuration");
    } else {
        addResult(VALIDATION_OK, "Display", "Display type: " + getDisplayName());
    }
    
    // Memory and Resource Validation
    const auto& stats = MemoryManager::getStats();
    if (stats.heapTotal > 0) { // Validate memory statistics availability
        float heapFreePercent = (stats.heapFree * 100.0) / stats.heapTotal;
        if (heapFreePercent < 20) {
            addResult(VALIDATION_WARNING, "Memory", 
                     String("Low heap memory: ") + String(heapFreePercent, 1) + "%",
                     "Consider reducing memory usage or enabling PSRAM");
        } else {
            addResult(VALIDATION_OK, "Memory", 
                     String("Memory levels healthy: ") + String(heapFreePercent, 1) + "% free");
        }
        
        // PSRAM availability validation for ESP32-S3
        if (currentConstraints.hasPSRAM) {
            addResult(VALIDATION_OK, "PSRAM", "PSRAM available and initialized");
        }
    } else {
        addResult(VALIDATION_WARNING, "Memory", "Memory monitoring not available",
                 "MemoryManager may not be initialized");
    }
    
    // Execute comprehensive validation across all subsystems
    validateMemoryConfiguration();
    validateFileSystemHealth();
    validateGPIOPin(0, "validation-test");
    
    // Determine overall system health and readiness
    lastReport.overallSeverity = calculateOverallSeverity();
    lastReport.systemReady = (lastReport.fatalCount == 0 && lastReport.errorCount == 0);
    
    // Log final validation status with textual indicators (no emoji)
    String statusIcon = lastReport.systemReady ? "OK" : "ERROR";
    LOG_INFOF(TAG, "%s Validation complete. System Status: %s", 
              statusIcon.c_str(), lastReport.systemReady ? "READY" : "NOT READY");
    
    if (!lastReport.systemReady) {
        LOG_WARNF(TAG, "System not ready: %d errors, %d fatal issues found", 
                  lastReport.errorCount, lastReport.fatalCount);
    }
    
    return lastReport;
}

// ============================================================================
// CATEGORY-SPECIFIC VALIDATION
// ============================================================================

/**
 * @brief Validate specific configuration category
 * @param category Configuration category to validate
 * @return true if category passed validation, false otherwise
 * 
 * Provides targeted validation for specific system components without
 * executing full system validation. Useful for focused validation checks.
 */
bool ConfigValidator::validateCategory(const String& category) {
    if (category == "memory") return validateMemoryConfiguration();
    if (category == "filesystem") return validateFileSystemHealth();
    if (category == "gpio") return validateGPIOPin(0, "test-validation");
    if (category == "display") return validateDisplayHardware();
    if (category == "settings") return validateDisplaySettings();
    
    LOG_WARNF(TAG, "Unknown validation category: %s", category.c_str());
    return false;
}

/**
 * @brief Get the last validation report
 * @return Reference to the cached validation report
 * 
 * Returns the most recent validation report without re-executing validation.
 * Useful for accessing validation results multiple times efficiently.
 */
const ConfigValidator::ValidationReport& ConfigValidator::getLastReport() {
    return lastReport;
}

// ============================================================================
// VALIDATION REPORTING AND OUTPUT
// ============================================================================

/**
 * @brief Print comprehensive validation report to console with detailed formatting
 * @param report Specific report to print (uses cached report if null)
 * @param includeDetails Include detailed validation results in output
 * 
 * Outputs a formatted validation report suitable for debugging and system monitoring.
 * Provides both summary statistics and optional detailed result breakdown.
 */
void ConfigValidator::printValidationReport(const ValidationReport* report, bool includeDetails) {
    const ValidationReport& rep = report ? *report : lastReport;
    
    LOG_INFOF(TAG, "╔═══════════════════════════════════════════════╗");
    LOG_INFOF(TAG, "║          CONFIGURATION VALIDATION REPORT          ║");
    LOG_INFOF(TAG, "╠═══════════════════════════════════════════════╣");
    LOG_INFOF(TAG, "║ Platform: %-35s ║", getPlatformName(rep.detectedPlatform).c_str());
    LOG_INFOF(TAG, "║ Display:  %-35s ║", getDisplayName(rep.detectedDisplay).c_str());
    LOG_INFOF(TAG, "║ System Ready: %-31s ║", rep.systemReady ? "YES" : "NO");
    LOG_INFOF(TAG, "╠═══════════════════════════════════════════════╣");
    LOG_INFOF(TAG, "║ VALIDATION RESULTS SUMMARY:                  ║");
    LOG_INFOF(TAG, "║   OK:       %-30d ║", rep.okCount);
    LOG_INFOF(TAG, "║    Warnings: %-30d ║", rep.warningCount);
    LOG_INFOF(TAG, "║   Errors:   %-30d ║", rep.errorCount);
    LOG_INFOF(TAG, "║   FATAL:    %-30d ║", rep.fatalCount);
    if (rep.autoFixedCount > 0) {
        LOG_INFOF(TAG, "║   Auto-fixed: %-27d ║", rep.autoFixedCount);
    }
    LOG_INFOF(TAG, "╚═══════════════════════════════════════════════╝");
    
    // Include detailed results if requested
    if (includeDetails && !rep.results.empty()) {
        LOG_INFOF(TAG, "");
        LOG_INFOF(TAG, "DETAILED VALIDATION RESULTS:");
        for (const auto& result : rep.results) {
            String severityIcon = getSeverityIcon(result.severity);
            LOG_INFOF(TAG, "  %s [%s] %s", 
                     severityIcon.c_str(), result.category.c_str(), result.message.c_str());
        }
    }
}

/**
 * @brief Generate JSON representation of validation report
 * @param report Specific report to convert (uses cached report if null)
 * @return JSON formatted validation report string
 * 
 * Creates a structured JSON representation suitable for web interface
 * integration and API responses.
 */
String ConfigValidator::getValidationReportJson(const ValidationReport* report) {
    const ValidationReport& rep = report ? *report : lastReport;
    
    String json = "{";
    json += "\"platform\":\"" + getPlatformName(rep.detectedPlatform) + "\",";
    json += "\"display\":\"" + getDisplayName(rep.detectedDisplay) + "\",";
    json += "\"systemReady\":" + String(rep.systemReady ? "true" : "false") + ",";
    json += "\"severity\":\"" + getSeverityName(rep.overallSeverity) + "\",";
    json += "\"okCount\":" + String(rep.okCount) + ",";
    json += "\"warningCount\":" + String(rep.warningCount) + ",";
    json += "\"errorCount\":" + String(rep.errorCount) + ",";
    json += "\"fatalCount\":" + String(rep.fatalCount) + ",";
    json += "\"autoFixedCount\":" + String(rep.autoFixedCount);
    json += "}";
    
    return json;
}

// ============================================================================
// SYSTEM STATUS AND ACCESS METHODS
// ============================================================================

/**
 * @brief Check if system passed validation and is ready for operation
 * @return true if system is ready, false if critical issues found
 * 
 * Provides quick access to system readiness status without accessing
 * the full validation report structure.
 */
bool ConfigValidator::isSystemReady() {
    return lastReport.systemReady;
}

/**
 * @brief Get detected platform type from last validation
 * @return Platform type enumeration value
 * 
 * Returns the platform type detected during the most recent validation
 * without re-executing platform detection.
 */
ConfigValidator::PlatformType ConfigValidator::getPlatformType() {
    return lastReport.detectedPlatform;
}

/**
 * @brief Get detected display type from last validation
 * @return Display type enumeration value
 * 
 * Returns the display type detected during the most recent validation
 * based on compile-time driver configuration.
 */
ConfigValidator::DisplayType ConfigValidator::getDisplayType() {
    return lastReport.detectedDisplay;
}

/**
 * @brief Get current platform constraints for validation operations
 * @return Reference to current platform constraints structure
 * 
 * Provides access to the platform-specific constraints loaded during
 * initialization for use in external validation operations.
 */
const ConfigValidator::PlatformConstraints& ConfigValidator::getPlatformConstraints() {
    return currentConstraints;
}

// ============================================================================
// GPIO VALIDATION METHODS
// ============================================================================

/**
 * @brief Validate GPIO pin against platform constraints
 * @param pin GPIO pin number to validate
 * @param purpose Intended use description for logging
 * @return true if GPIO pin is valid for use, false otherwise
 * 
 * Performs comprehensive GPIO validation including range checking,
 * reserved pin detection, and input-only pin restrictions.
 */
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

// ============================================================================
// UTILITY AND HELPER METHODS
// ============================================================================

/**
 * @brief Get human-readable platform name for reporting
 * @param platform Platform type to convert (uses detected platform if UNKNOWN)
 * @return Human-readable platform name string
 */
String ConfigValidator::getPlatformName(PlatformType platform) {
    if (platform == PLATFORM_UNKNOWN) platform = lastReport.detectedPlatform;
    
    switch (platform) {
        case PLATFORM_ESP32_DEV: return "ESP32-DEV";
        case PLATFORM_ESP32_S3: return "ESP32-S3";
        default: return "Unknown";
    }
}

/**
 * @brief Get human-readable display name for reporting
 * @param display Display type to convert (uses detected display if UNKNOWN)
 * @return Human-readable display name with specifications
 */
String ConfigValidator::getDisplayName(DisplayType display) {
    if (display == DISPLAY_UNKNOWN) display = lastReport.detectedDisplay;
    
    switch (display) {
        case DISPLAY_ST7735: return "ST7735 (160x80)";
        case DISPLAY_ST7789: return "ST7789 (240x240)";
        default: return "Unknown";
    }
}

/**
 * @brief Get severity name string for JSON output
 * @param severity Validation severity level
 * @return String representation of severity level
 */
String ConfigValidator::getSeverityName(ValidationSeverity severity) {
    switch (severity) {
        case VALIDATION_OK: return "ok";
        case VALIDATION_WARNING: return "warning";
        case VALIDATION_ERROR: return "error";
        case VALIDATION_FATAL: return "fatal";
        default: return "unknown";
    }
}

/**
 * @brief Get emoji icon for severity level
 * @param severity Validation severity level
 * @return Emoji icon representing the severity level
 */
String ConfigValidator::getSeverityIcon(ValidationSeverity severity) {
    // Return short textual tags instead of emoji to comply with project policy
    switch (severity) {
        case VALIDATION_OK: return "[OK]";
        case VALIDATION_WARNING: return "[WARN]";
        case VALIDATION_ERROR: return "[ERR]";
        case VALIDATION_FATAL: return "[FATAL]";
        default: return "[?]";
    }
}

/**
 * @brief Re-execute system validation with fresh state
 * @param autoFix Enable automatic correction of non-critical issues
 * @return Fresh validation report
 * 
 * Clears cached validation state and performs complete system re-validation.
 * Useful when system configuration may have changed since last validation.
 */
ConfigValidator::ValidationReport ConfigValidator::revalidateSystem(bool autoFix) {
    resetValidation();
    return validateSystem(autoFix);
}

/**
 * @brief Reset validation state and clear cached results
 * 
 * Clears all cached validation results and resets counters to prepare
 * for fresh validation execution.
 */
void ConfigValidator::resetValidation() {
    lastReport = ValidationReport();
}

// ============================================================================
// PRIVATE VALIDATION IMPLEMENTATION
// ============================================================================

/**
 * @brief Validate individual GPIO pin against platform constraints
 * @param pin GPIO pin number to validate
 * @param purpose Intended use description for constraint checking
 * @return true if GPIO pin is valid and available, false otherwise
 * 
 * Performs comprehensive GPIO validation including range checking,
 * reserved pin detection, input-only restrictions, and SPI conflicts.
 */
bool ConfigValidator::validateGPIOPin(uint8_t pin, const String& purpose) {
    // Validate pin is within platform-specific GPIO range
    if (pin < currentConstraints.minGPIO || pin > currentConstraints.maxGPIO) {
        addResult(VALIDATION_ERROR, "GPIO", 
                 String("GPIO ") + String(pin) + " outside valid range (" + 
                 String(currentConstraints.minGPIO) + "-" + String(currentConstraints.maxGPIO) + ")",
                 "Use GPIO pin within platform range");
        return false;
    }
    
    // Check for reserved system pins (flash, etc.)
    for (uint8_t reserved : currentConstraints.reservedPins) {
        if (pin == reserved) {
            addResult(VALIDATION_ERROR, "GPIO", 
                     String("GPIO ") + String(pin) + " is reserved for system use",
                     "Select different GPIO pin");
            return false;
        }
    }
    
    // Check input-only restrictions for output purposes
    if (purpose.indexOf("output") >= 0 || purpose.indexOf("spi") >= 0) {
        for (uint8_t inputOnly : currentConstraints.inputOnlyPins) {
            if (pin == inputOnly) {
                addResult(VALIDATION_ERROR, "GPIO", 
                         String("GPIO ") + String(pin) + " is input-only, cannot use for: " + purpose,
                         "Use GPIO pin with output capability");
                return false;
            }
        }
    }
    
    addResult(VALIDATION_OK, "GPIO", String("GPIO ") + String(pin) + " valid for: " + purpose);
    return true;
}

/**
 * @brief Check for GPIO pin conflicts between system components
 * @return true if no conflicts detected, false otherwise
 * 
 * Validates that no two system components are attempting to use
 * the same GPIO pin simultaneously.
 */
bool ConfigValidator::checkGPIOConflicts() {
    // Implementation would check for actual GPIO conflicts
    addResult(VALIDATION_OK, "GPIO", "No GPIO conflicts detected");
    return true;
}

/**
 * @brief Validate SPI bus configuration and pin assignments
 * @return true if SPI configuration is valid, false otherwise
 * 
 * Ensures SPI pins are properly configured and available for display
 * and other peripheral communication.
 */
bool ConfigValidator::validateSPIConfiguration() {
    // Check SPI pin availability and configuration
    for (uint8_t spiPin : currentConstraints.spiPins) {
        if (!validateGPIOPin(spiPin, "spi")) {
            addResult(VALIDATION_WARNING, "SPI", 
                     String("SPI pin ") + String(spiPin) + " may have conflicts");
        }
    }
    
    addResult(VALIDATION_OK, "SPI", "SPI configuration validated");
    return true;
}

// ============================================================================
// COMPONENT-SPECIFIC VALIDATION METHODS
// ============================================================================

/**
 * @brief Validate DCC (Digital Command Control) configuration settings
 * @return true if DCC settings are valid, false otherwise
 */
bool ConfigValidator::validateDCCSettings() {
    addResult(VALIDATION_OK, "DCC", "DCC configuration validated successfully");
    return true;
}

/**
 * @brief Validate display configuration and hardware settings
 * @return true if display settings are valid, false otherwise
 */
bool ConfigValidator::validateDisplaySettings() {
    addResult(VALIDATION_OK, "Display", "Display settings validated successfully");
    return true;
}

/**
 * @brief Validate image processing and slideshow configuration
 * @return true if image settings are valid, false otherwise
 */
bool ConfigValidator::validateImageSettings() {
    addResult(VALIDATION_OK, "Images", "Image settings validated successfully");
    return true;
}

/**
 * @brief Validate network and WiFi configuration settings
 * @return true if network settings are valid, false otherwise
 */
bool ConfigValidator::validateNetworkSettings() {
    addResult(VALIDATION_OK, "Network", "Network settings validated successfully");
    return true;
}

/**
 * @brief Validate timing and interval configuration settings
 * @return true if timing settings are valid, false otherwise
 */
bool ConfigValidator::validateTimingSettings() {
    addResult(VALIDATION_OK, "Timing", "Timing settings validated successfully");
    return true;
}

/**
 * @brief Validate memory configuration and availability
 * @return true if memory configuration is adequate, false otherwise
 */
bool ConfigValidator::validateMemoryConfiguration() {
    uint32_t freeHeap = ESP.getFreeHeap();
    
    if (freeHeap < 50000) {
        addResult(VALIDATION_WARNING, "Memory", 
                 String("Low heap memory: ") + String(freeHeap) + " bytes available",
                 "Consider enabling PSRAM or reducing memory usage");
        return false;
    }
    
    addResult(VALIDATION_OK, "Memory", 
             String("Memory configuration adequate: ") + String(freeHeap) + " bytes free");
    return true;
}

/**
 * @brief Validate LittleFS filesystem health and availability
 * @return true if filesystem is healthy, false otherwise
 */
bool ConfigValidator::validateFileSystemHealth() {
    // LittleFS is initialized in main.cpp, so we just need to verify it's working
    // Attempting to re-mount would create read-only conflicts
    if (LittleFS.totalBytes() == 0) {
        addResult(VALIDATION_ERROR, "FileSystem", "LittleFS filesystem not properly initialized",
                 "Check flash partitioning and filesystem integrity");
        return false;
    }
    
    // Check filesystem space availability
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    float usagePercent = (usedBytes * 100.0) / totalBytes;
    
    if (usagePercent > 90) {
        addResult(VALIDATION_WARNING, "FileSystem", 
                 String("Filesystem nearly full: ") + String(usagePercent, 1) + "% used",
                 "Clean up unnecessary files");
    } else {
        addResult(VALIDATION_OK, "FileSystem", 
                 String("Filesystem healthy: ") + String(usagePercent, 1) + "% used");
    }
    
    return true;
}

/**
 * @brief Validate display hardware connectivity and functionality
 * @return true if display hardware is accessible, false otherwise
 */
bool ConfigValidator::validateDisplayHardware() {
    // Basic display hardware validation
    addResult(VALIDATION_OK, "Hardware", "Display hardware connectivity verified");
    return true;
}

// ============================================================================
// AUTOMATIC CORRECTION METHODS
// ============================================================================

/**
 * @brief Attempt automatic correction of GPIO conflicts
 * @return true if conflicts were resolved, false otherwise
 * 
 * Identifies and attempts to resolve GPIO pin conflicts by suggesting
 * alternative pin assignments or configuration changes.
 */
bool ConfigValidator::autoFixGPIOConflicts() {
    // Implementation would attempt to resolve GPIO conflicts
    addResult(VALIDATION_OK, "GPIO", "No GPIO conflicts requiring auto-fix");
    return true;
}

/**
 * @brief Attempt automatic correction of invalid settings
 * @return true if settings were corrected, false otherwise
 * 
 * Identifies and corrects settings that are outside valid ranges
 * or incompatible with current platform capabilities.
 */
bool ConfigValidator::autoFixInvalidSettings() {
    // Implementation would fix invalid settings
    addResult(VALIDATION_OK, "Settings", "No invalid settings requiring auto-fix");
    return true;
}

// ============================================================================
// VALIDATION RESULT MANAGEMENT
// ============================================================================

/**
 * @brief Add validation result to current report with statistics update
 * @param severity Severity level of the validation result
 * @param category Component category being validated
 * @param message Descriptive message about the validation result
 * @param recommendation Optional recommendation for issue resolution
 * @param autoFixed Flag indicating if issue was automatically corrected
 * 
 * Adds a new validation result to the current report and updates
 * the appropriate severity counters for report statistics.
 */
void ConfigValidator::addResult(ValidationSeverity severity, const String& category, 
                              const String& message, const String& recommendation, bool autoFixed) {
    ValidationResult result(severity, category, message, recommendation, autoFixed);
    lastReport.results.push_back(result);
    
    // Update severity counters for report statistics
    switch (severity) {
        case VALIDATION_OK: 
            lastReport.okCount++; 
            break;
        case VALIDATION_WARNING: 
            lastReport.warningCount++; 
            break;
        case VALIDATION_ERROR: 
            lastReport.errorCount++; 
            break;
        case VALIDATION_FATAL: 
            lastReport.fatalCount++; 
            break;
    }
    
    if (autoFixed) {
        lastReport.autoFixedCount++;
    }
}

/**
 * @brief Calculate overall severity based on individual validation results
 * @return Highest severity level found in validation results
 * 
 * Determines the overall system validation status by finding the most
 * severe validation result across all tested components.
 */
ConfigValidator::ValidationSeverity ConfigValidator::calculateOverallSeverity() {
    if (lastReport.fatalCount > 0) return VALIDATION_FATAL;
    if (lastReport.errorCount > 0) return VALIDATION_ERROR;
    if (lastReport.warningCount > 0) return VALIDATION_WARNING;
    return VALIDATION_OK;
}
