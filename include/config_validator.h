/**
 * @file config_validator.h
 * @brief Multiplatform configuration validation system for ESP32 Billboard
 * 
 * This module provides comprehensive validation for all supported hardware platforms,
 * ensuring safe and optimal configuration across different ESP32 variants and display types.
 * 
 * Supported Platforms:
 * - ESP32-DEV (standard ESP32)
 * - ESP32-S3 (with optional PSRAM)
 * 
 * Supported Displays:
 * - ST7735 (160x80, dual display support)
 * - ST7789 (240x240, active support)
 * 
 * Key Features:
 * - Platform-specific GPIO validation
 * - Hardware capability detection
 * - Settings range validation
 * - Auto-correction of invalid configurations
 * - Comprehensive startup validation report
 * 
 * @author ESP32-S3 Billboard System
 * @date 2025
 * @version 1.0.0
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include "logger.h"
#include "config.h"

/**
 * @class ConfigValidator
 * @brief Comprehensive multiplatform configuration validation system
 * 
 * Validates all system configurations against platform-specific capabilities,
 * GPIO constraints, and hardware requirements. Provides automatic error
 * correction and detailed validation reporting.
 */
class ConfigValidator {
public:
    /**
     * @brief Platform identification enumeration
     */
    enum PlatformType {
        PLATFORM_ESP32_DEV = 0,    ///< Standard ESP32 development board
        PLATFORM_ESP32_S3 = 1,     ///< ESP32-S3 with enhanced capabilities
        PLATFORM_UNKNOWN = 255     ///< Unknown or unsupported platform
    };

    /**
     * @brief Display driver enumeration
     */
    enum DisplayType {
        DISPLAY_ST7735 = 0,         ///< ST7735 160x80 displays
        DISPLAY_ST7789 = 1,         ///< ST7789 240x240 displays
        DISPLAY_UNKNOWN = 255       ///< Unknown display type
    };

    /**
     * @brief Validation severity levels
     */
    enum ValidationSeverity {
        VALIDATION_OK = 0,          ///< Configuration is valid
        VALIDATION_WARNING = 1,     ///< Non-critical issues found
        VALIDATION_ERROR = 2,       ///< Critical issues found
        VALIDATION_FATAL = 3        ///< Fatal errors, system cannot start
    };

    /**
     * @brief Individual validation result
     */
    struct ValidationResult {
        ValidationSeverity severity;
        String category;
        String message;
        String recommendation;
        bool autoFixed;

        ValidationResult(ValidationSeverity sev, const String& cat, const String& msg, const String& rec = "", bool fixed = false)
            : severity(sev), category(cat), message(msg), recommendation(rec), autoFixed(fixed) {}
    };

    /**
     * @brief Comprehensive validation report
     */
    struct ValidationReport {
        PlatformType detectedPlatform;
        DisplayType detectedDisplay;
        bool systemReady;
        ValidationSeverity overallSeverity;
        
        std::vector<ValidationResult> results;
        
        // Statistics
        uint8_t okCount;
        uint8_t warningCount;
        uint8_t errorCount;
        uint8_t fatalCount;
        uint8_t autoFixedCount;
        
        ValidationReport() : detectedPlatform(PLATFORM_UNKNOWN), detectedDisplay(DISPLAY_UNKNOWN), 
                           systemReady(false), overallSeverity(VALIDATION_OK),
                           okCount(0), warningCount(0), errorCount(0), fatalCount(0), autoFixedCount(0) {}
    };

    /**
     * @brief Platform-specific GPIO constraints
     */
    struct PlatformConstraints {
        uint8_t minGPIO;            ///< Minimum valid GPIO number
        uint8_t maxGPIO;            ///< Maximum valid GPIO number
        std::vector<uint8_t> reservedPins;     ///< System reserved pins
        std::vector<uint8_t> inputOnlyPins;    ///< Input-only pins
        std::vector<uint8_t> spiPins;          ///< SPI bus pins
        bool hasPSRAM;              ///< Platform supports PSRAM
        uint32_t maxRAM;            ///< Maximum RAM available
        String platformName;        ///< Human-readable platform name
    };

private:
    static const char* TAG;
    static ValidationReport lastReport;
    static PlatformConstraints currentConstraints;
    
    // Platform detection
    static PlatformType detectPlatform();
    static DisplayType detectDisplayType();
    static void loadPlatformConstraints(PlatformType platform);
    
    // GPIO validation
    static bool validateGPIOPin(uint8_t pin, const String& purpose);
    static bool checkGPIOConflicts();
    static bool validateSPIConfiguration();
    
    // Settings validation
    static bool validateDCCSettings();
    static bool validateDisplaySettings();
    static bool validateImageSettings();
    static bool validateNetworkSettings();
    static bool validateTimingSettings();
    
    // Hardware validation
    static bool validateMemoryConfiguration();
    static bool validateFileSystemHealth();
    static bool validateDisplayHardware();
    
    // Auto-correction functions
    static bool autoFixGPIOConflicts();
    static bool autoFixInvalidSettings();
    
    // Validation helpers
    static void addResult(ValidationSeverity severity, const String& category, 
                         const String& message, const String& recommendation = "", bool autoFixed = false);
    static ValidationSeverity calculateOverallSeverity();

public:
    /**
     * @brief Initialize the configuration validator
     * @return true if validator initialized successfully
     */
    static bool initialize();

    /**
     * @brief Perform comprehensive system validation
     * @param autoFix Enable automatic fixing of correctable issues (default: true)
     * @return Comprehensive validation report
     */
    static ValidationReport validateSystem(bool autoFix = true);

    /**
     * @brief Validate specific configuration category
     * @param category Configuration category to validate
     * @return true if category passed validation
     */
    static bool validateCategory(const String& category);

    /**
     * @brief Get the last validation report
     * @return Reference to the last validation report
     */
    static const ValidationReport& getLastReport();

    /**
     * @brief Print validation report to console
     * @param report Report to print (uses last report if not specified)
     * @param includeDetails Include detailed results in output
     */
    static void printValidationReport(const ValidationReport* report = nullptr, bool includeDetails = true);

    /**
     * @brief Get validation report as JSON string
     * @param report Report to convert (uses last report if not specified)
     * @return JSON formatted validation report
     */
    static String getValidationReportJson(const ValidationReport* report = nullptr);

    /**
     * @brief Check if system passed validation and is ready to operate
     * @return true if system is ready, false if critical issues found
     */
    static bool isSystemReady();

    /**
     * @brief Get detected platform type
     * @return Detected platform type
     */
    static PlatformType getPlatformType();

    /**
     * @brief Get detected display type
     * @return Detected display type
     */
    static DisplayType getDisplayType();

    /**
     * @brief Get platform constraints for current platform
     * @return Platform-specific constraints
     */
    static const PlatformConstraints& getPlatformConstraints();

    /**
     * @brief Validate a specific GPIO pin for a purpose
     * @param pin GPIO pin number to validate
     * @param purpose Description of intended use
     * @return true if pin is valid for the purpose
     */
    static bool isValidGPIO(uint8_t pin, const String& purpose = "general");

    /**
     * @brief Get list of available GPIO pins for a specific purpose
     * @param purpose Purpose description (affects filtering)
     * @return Vector of available GPIO pin numbers
     */
    static std::vector<uint8_t> getAvailableGPIOs(const String& purpose = "general");

    /**
     * @brief Get human-readable platform name
     * @param platform Platform type (uses detected if not specified)
     * @return Platform name string
     */
    static String getPlatformName(PlatformType platform = PLATFORM_UNKNOWN);

    /**
     * @brief Get human-readable display type name
     * @param display Display type (uses detected if not specified)
     * @return Display type name string
     */
    static String getDisplayName(DisplayType display = DISPLAY_UNKNOWN);

    /**
     * @brief Force re-validation of the entire system
     * @param autoFix Enable automatic fixing
     * @return New validation report
     */
    static ValidationReport revalidateSystem(bool autoFix = true);

    /**
     * @brief Reset validation state and clear cached results
     */
    static void resetValidation();
};

// Convenience macros for validation
#define CONFIG_VALIDATE() ConfigValidator::validateSystem()
#define CONFIG_IS_READY() ConfigValidator::isSystemReady()
#define CONFIG_PRINT_REPORT() ConfigValidator::printValidationReport()
#define CONFIG_GET_PLATFORM() ConfigValidator::getPlatformType()
#define CONFIG_VALIDATE_GPIO(pin, purpose) ConfigValidator::isValidGPIO(pin, purpose)
#define CONFIG_RESET() ConfigValidator::resetValidation()
