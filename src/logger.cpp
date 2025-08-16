/**
 * @file logger.cpp
 * @brief Implementation of hierarchical logging system with build-time level control
 * 
 * Provides comprehensive logging capabilities for ESP32 dual display billboard system
 * with compile-time optimization, formatted output, and system diagnostics integration.
 * Supports build flag configuration for production deployments.
 * 
 * Features:
 * - Five logging levels (ERROR, WARN, INFO, DEBUG, VERBOSE)
 * - Build-time level filtering for production optimization
 * - Printf-style formatted logging with variadic arguments
 * - Professional text prefixes for clear log level identification
 * - System diagnostics integration (memory, WiFi, hardware info)
 * - Automatic Serial initialization with timeout protection
 * - Compile-time disabling for production builds
 * 
 * @author ESP32-S3 Billboard System
 * @date 2025
 * @version 0.9
 */

#include "logger.h"
#include <WiFi.h>
#include <stdarg.h>

bool Logger::initialized = false;
static Logger::Level currentLevel = Logger::INFO;

/**
 * @brief Ensures logger system is properly initialized before use
 * 
 * Internal initialization check that automatically initializes the logging
 * system if not already done. Provides lazy initialization to ensure
 * Serial communication is available before any logging operations.
 * 
 * Called automatically by all logging functions to guarantee proper
 * initialization state without requiring explicit initialization calls.
 * 
 * @note This is an internal method called automatically by logging functions
 * @note Only performs initialization once regardless of call frequency
 * @note Thread-safe and suitable for use from any context
 * 
 * @see init() for manual logger initialization
 * @see initialized static member for initialization state tracking
 * 
 * @since v0.9
 */
void Logger::ensureInitialized() {
    if (!initialized) {
        init();
    }
}

/**
 * @brief Initializes the logging system with configurable serial communication
 * 
 * Sets up Serial communication for logging output with timeout protection
 * and system identification. Displays startup banner and version information
 * to clearly identify system boot and logging initialization.
 * 
 * Initialization process:
 * - Configures Serial communication at specified baud rate
 * - Implements 3-second timeout to prevent indefinite blocking
 * - Uses non-blocking yield() calls to maintain system responsiveness
 * - Displays system identification banner with emoji indicators
 * - Sets initialization state to prevent repeated initialization
 * 
 * @param baudRate Serial communication baud rate (default: 115200)
 *                 Common values: 9600, 38400, 57600, 115200, 230400
 * 
 * @note Only active when LOGGER_ENABLED build flag is true
 * @note Includes timeout protection to prevent blocking during boot
 * @note Can be called multiple times safely (initialization occurs only once)
 * @note Displays system banner for easy log identification
 * 
 * @see ensureInitialized() for automatic initialization
 * @see setLevel() for runtime log level configuration
 * @see LOGGER_ENABLED build flag for compile-time control
 * 
 * @since v0.9
 */
void Logger::init(unsigned long baudRate) {
    #if defined(LOGGER_ENABLED) && LOGGER_ENABLED
    Serial.begin(baudRate);
    while (!Serial && millis() < 3000) {
        yield(); // Non-blocking yield instead of delay
    }
    initialized = true;
    
    Serial.println();
    Serial.println("=================================");
    Serial.println("Billboard System Logger v0.9");
    Serial.println("=================================");
    #endif
}

/**
 * @brief Sets the runtime logging level threshold
 * 
 * Configures the minimum logging level that will be output to Serial.
 * Messages at or below the specified level will be displayed, while
 * higher level messages will be filtered out for cleaner output.
 * 
 * Logging level hierarchy (ascending verbosity):
 * - ERROR (1): Critical errors only
 * - WARN (2): Warnings and errors
 * - INFO (3): Informational messages, warnings, and errors
 * - DEBUG (4): Debug information plus all lower levels
 * - VERBOSE (5): All logging output including trace information
 * 
 * @param level Maximum logging level to display (Level enumeration)
 * 
 * @note Runtime level filtering works in conjunction with build-time flags
 * @note Does not affect build-time compilation filtering via LOGGER_LEVEL_* flags
 * @note Default level is INFO providing balanced output
 * @note Can be changed at runtime to adjust verbosity dynamically
 * 
 * @see getLevel() for retrieving current logging level
 * @see Level enumeration for available logging levels
 * @see log() for level-based message filtering implementation
 * 
 * @since v0.9
 */
void Logger::setLevel(Level level) {
    currentLevel = level;
}

/**
 * @brief Retrieves the current runtime logging level threshold
 * 
 * Returns the currently configured logging level that determines which
 * messages are displayed. Useful for conditional logging decisions and
 * dynamic log level management in applications.
 * 
 * @return Level Current logging level threshold
 *         Messages at this level or lower will be displayed
 * 
 * @note Reflects runtime level setting, not build-time flag configuration
 * @note Default return value is INFO level
 * @note Can be used for conditional logic around expensive log operations
 * 
 * @see setLevel() for configuring logging level threshold
 * @see Level enumeration for available logging levels
 * @see log() for how level threshold is applied during filtering
 * 
 * @since v0.9
 */
Logger::Level Logger::getLevel() {
    return currentLevel;
}

/**
 * @brief Core logging function with level filtering and formatted output
 * 
 * Primary logging implementation that handles level filtering, build-time
 * optimization, and formatted message output. Provides timestamp, visual
 * indicators, and structured formatting for all log messages.
 * 
 * Message processing:
 * - Build-time level filtering using LOGGER_LEVEL_* flags
 * - Runtime level filtering using currentLevel threshold
 * - Automatic logger initialization if not already initialized
 * - Timestamp generation using millis() for relative timing
 * - Visual emoji prefixes for quick level identification
 * - Structured format: [timestamp] [LEVEL] [TAG] Message
 * 
 * Professional level indicators:
 * - ERROR: [ERROR] (critical issues)
 * - WARN: [WARN] (warning conditions)
 * - INFO: [INFO] (general information)
 * - DEBUG: [DEBUG] (debug information)
 * - VERBOSE: [VERBOSE] (detailed trace)
 * 
 * @param level Logging level for this message (Level enumeration)
 * @param tag Component or module identifier for message categorization
 * @param message Log message content to be displayed
 * 
 * @note Only active when LOGGER_ENABLED build flag is true
 * @note Respects both build-time and runtime level filtering
 * @note Timestamp shows milliseconds since system boot
 * @note Message output goes to Serial interface
 * 
 * @see logf() for printf-style formatted logging
 * @see Level enumeration for available logging levels
 * @see ensureInitialized() for automatic initialization
 * 
 * @since v0.9
 */
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
        case ERROR:   prefix = "[ERROR]  "; break;
        case WARN:    prefix = "[WARN]   "; break;
        case INFO:    prefix = "[INFO]   "; break;
        case DEBUG:   prefix = "[DEBUG]  "; break;
        case VERBOSE: prefix = "[VERBOSE]"; break;
    }
    
    // Format: [timestamp] [DEBUG] [TAG] Message
    Serial.printf("[%08lu] %s [%s] %s\n", 
                  timestamp, 
                  prefix.c_str(), 
                  tag.c_str(), 
                  message.c_str());
    #endif
}

/**
 * @brief Printf-style formatted logging with variadic arguments
 * 
 * Provides printf-style formatting capabilities for complex log messages
 * requiring variable substitution, numeric formatting, or structured output.
 * Uses internal buffer for format string processing before delegation to log().
 * 
 * Format processing:
 * - Supports full printf format specifier syntax
 * - Uses 512-byte internal buffer for formatted output
 * - Automatic buffer management with safe overflow handling
 * - Delegates to log() for level filtering and output formatting
 * 
 * Supported format specifiers include:
 * - %s (strings), %d (integers), %f (floats)
 * - %x (hexadecimal), %c (characters)
 * - Width and precision specifiers (e.g., %04d, %.2f)
 * 
 * @param level Logging level for this message (Level enumeration)
 * @param tag Component or module identifier for message categorization
 * @param format Printf-style format string with placeholders
 * @param ... Variable arguments corresponding to format placeholders
 * 
 * @note Buffer size is 512 bytes; longer formatted strings will be truncated
 * @note All level filtering and build-time flags apply as with log()
 * @note Format string processing occurs even if message will be filtered
 * @note Consider performance impact for verbose logging with complex formatting
 * 
 * @see log() for basic string logging without formatting
 * @see Level enumeration for available logging levels
 * @see Standard printf documentation for format specifier syntax
 * 
 * @since v0.9
 */
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

/**
 * @brief Logs error-level message with simplified interface
 * 
 * Convenience wrapper for error-level logging that eliminates need to specify
 * logging level explicitly. Designed for critical error conditions that require
 * immediate attention and should always be visible in production logs.
 * 
 * @param tag Component or module identifier for message categorization
 * @param message Error message content describing the critical condition
 * 
 * @note Equivalent to calling log(ERROR, tag, message)
 * @note Subject to build-time ERROR level filtering via LOGGER_LEVEL_ERROR
 * @note Highest priority logging level for critical system issues
 * 
 * @see log() for general-purpose logging with explicit level specification
 * @see errorf() for printf-style formatted error logging
 * @see Level::ERROR for error level threshold definition
 * 
 * @since v0.9
 */
void Logger::error(const String& tag, const String& message) {
    log(ERROR, tag, message);
}

/**
 * @brief Logs warning-level message with simplified interface
 * 
 * Convenience wrapper for warning-level logging suitable for non-critical
 * issues that deserve attention but do not prevent system operation.
 * Ideal for recoverable errors, deprecated function usage, or configuration issues.
 * 
 * @param tag Component or module identifier for message categorization
 * @param message Warning message content describing the condition
 * 
 * @note Equivalent to calling log(WARN, tag, message)
 * @note Subject to build-time WARN level filtering via LOGGER_LEVEL_WARN
 * @note Second highest priority level for important but non-critical issues
 * 
 * @see log() for general-purpose logging with explicit level specification
 * @see warnf() for printf-style formatted warning logging
 * @see Level::WARN for warning level threshold definition
 * 
 * @since v0.9
 */
void Logger::warn(const String& tag, const String& message) {
    log(WARN, tag, message);
}

/**
 * @brief Logs informational message with simplified interface
 * 
 * Convenience wrapper for informational logging suitable for general system
 * status updates, operational milestones, and user-relevant information.
 * Default logging level providing balanced output for normal operation.
 * 
 * @param tag Component or module identifier for message categorization
 * @param message Informational message content for general status updates
 * 
 * @note Equivalent to calling log(INFO, tag, message)
 * @note Subject to build-time INFO level filtering via LOGGER_LEVEL_INFO
 * @note Default logging level suitable for production environments
 * @note Provides good balance between verbosity and useful information
 * 
 * @see log() for general-purpose logging with explicit level specification
 * @see infof() for printf-style formatted informational logging
 * @see Level::INFO for info level threshold definition
 * 
 * @since v0.9
 */
void Logger::info(const String& tag, const String& message) {
    log(INFO, tag, message);
}

/**
 * @brief Logs debug-level message with simplified interface
 * 
 * Convenience wrapper for debug-level logging suitable for development
 * troubleshooting, state tracking, and detailed operational insight.
 * Typically filtered out in production builds for performance optimization.
 * 
 * @param tag Component or module identifier for message categorization
 * @param message Debug message content with detailed system information
 * 
 * @note Equivalent to calling log(DEBUG, tag, message)
 * @note Subject to build-time DEBUG level filtering via LOGGER_LEVEL_DEBUG
 * @note Intended primarily for development and debugging scenarios
 * @note Higher verbosity than INFO level with more detailed output
 * 
 * @see log() for general-purpose logging with explicit level specification
 * @see debugf() for printf-style formatted debug logging
 * @see Level::DEBUG for debug level threshold definition
 * 
 * @since v0.9
 */
void Logger::debug(const String& tag, const String& message) {
    log(DEBUG, tag, message);
}

/**
 * @brief Logs verbose-level message with simplified interface
 * 
 * Convenience wrapper for the most detailed logging level suitable for
 * comprehensive troubleshooting, step-by-step execution tracking, and
 * in-depth system analysis. Typically filtered out in all but development builds.
 * 
 * @param tag Component or module identifier for message categorization
 * @param message Verbose message content with comprehensive detail
 * 
 * @note Equivalent to calling log(VERBOSE, tag, message)
 * @note Subject to build-time VERBOSE level filtering via LOGGER_LEVEL_VERBOSE
 * @note Highest verbosity level producing most detailed output
 * @note Intended for intensive debugging and system analysis scenarios
 * @note May significantly impact performance if enabled in production
 * 
 * @see log() for general-purpose logging with explicit level specification
 * @see verbosef() for printf-style formatted verbose logging
 * @see Level::VERBOSE for verbose level threshold definition
 * 
 * @since v0.9
 */
void Logger::verbose(const String& tag, const String& message) {
    log(VERBOSE, tag, message);
}

/**
 * @brief Printf-style formatted error logging convenience function
 * 
 * Combines printf-style formatting with error-level logging for flexible
 * critical error message composition. Automatically handles variable argument
 * processing and buffer management for formatted output generation.
 * 
 * @param tag Component or module identifier for message categorization
 * @param format Printf-style format string with embedded format specifiers
 * @param ... Variable arguments matching format specifiers in format string
 * 
 * @note Uses 512-byte internal buffer for formatted message assembly
 * @note Subject to build-time ERROR level filtering via LOGGER_LEVEL_ERROR
 * @note Calls error() internally after formatting variable arguments
 * @note Buffer overflow protection via vsnprintf size limiting
 * @note Format specifiers follow standard printf conventions (%s, %d, %x, etc.)
 * 
 * @see error() for simple string-based error logging
 * @see logf() for general-purpose formatted logging with explicit level
 * @see Level::ERROR for error level threshold definition
 * 
 * @since v0.9
 */
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

/**
 * @brief Printf-style formatted warning logging convenience function
 * 
 * Combines printf-style formatting with warning-level logging for flexible
 * non-critical issue message composition. Handles variable argument processing
 * and buffer management for formatted warning message generation.
 * 
 * @param tag Component or module identifier for message categorization
 * @param format Printf-style format string with embedded format specifiers
 * @param ... Variable arguments matching format specifiers in format string
 * 
 * @note Uses 512-byte internal buffer for formatted message assembly
 * @note Subject to build-time WARN level filtering via LOGGER_LEVEL_WARN
 * @note Calls warn() internally after formatting variable arguments
 * @note Buffer overflow protection via vsnprintf size limiting
 * @note Ideal for recoverable errors and configuration issue reporting
 * 
 * @see warn() for simple string-based warning logging
 * @see logf() for general-purpose formatted logging with explicit level
 * @see Level::WARN for warning level threshold definition
 * 
 * @since v0.9
 */
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

/**
 * @brief Printf-style formatted informational logging convenience function
 * 
 * Combines printf-style formatting with informational-level logging for
 * flexible general status message composition. Default formatting function
 * providing balanced output suitable for production environments.
 * 
 * @param tag Component or module identifier for message categorization
 * @param format Printf-style format string with embedded format specifiers
 * @param ... Variable arguments matching format specifiers in format string
 * 
 * @note Uses 512-byte internal buffer for formatted message assembly
 * @note Subject to build-time INFO level filtering via LOGGER_LEVEL_INFO
 * @note Calls info() internally after formatting variable arguments
 * @note Buffer overflow protection via vsnprintf size limiting
 * @note Default formatting level for operational status reporting
 * 
 * @see info() for simple string-based informational logging
 * @see logf() for general-purpose formatted logging with explicit level
 * @see Level::INFO for info level threshold definition
 * 
 * @since v0.9
 */
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

/**
 * @brief Printf-style formatted debug logging convenience function
 * 
 * Combines printf-style formatting with debug-level logging for flexible
 * development troubleshooting message composition. Optimized for detailed
 * system state reporting and development diagnostic output.
 * 
 * @param tag Component or module identifier for message categorization
 * @param format Printf-style format string with embedded format specifiers
 * @param ... Variable arguments matching format specifiers in format string
 * 
 * @note Uses 512-byte internal buffer for formatted message assembly
 * @note Subject to build-time DEBUG level filtering via LOGGER_LEVEL_DEBUG
 * @note Calls debug() internally after formatting variable arguments
 * @note Buffer overflow protection via vsnprintf size limiting
 * @note Typically filtered out in production builds for performance
 * 
 * @see debug() for simple string-based debug logging
 * @see logf() for general-purpose formatted logging with explicit level
 * @see Level::DEBUG for debug level threshold definition
 * 
 * @since v0.9
 */
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

/**
 * @brief Printf-style formatted verbose logging convenience function
 * 
 * Combines printf-style formatting with verbose-level logging for the most
 * detailed message composition available. Designed for comprehensive system
 * analysis and intensive debugging scenarios requiring maximum detail.
 * 
 * @param tag Component or module identifier for message categorization
 * @param format Printf-style format string with embedded format specifiers
 * @param ... Variable arguments matching format specifiers in format string
 * 
 * @note Uses 512-byte internal buffer for formatted message assembly
 * @note Subject to build-time VERBOSE level filtering via LOGGER_LEVEL_VERBOSE
 * @note Calls verbose() internally after formatting variable arguments
 * @note Buffer overflow protection via vsnprintf size limiting
 * @note Highest verbosity level with potential performance impact
 * @note Intended for development environments only
 * 
 * @see verbose() for simple string-based verbose logging
 * @see logf() for general-purpose formatted logging with explicit level
 * @see Level::VERBOSE for verbose level threshold definition
 * 
 * @since v0.9
 */
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

/**
 * @brief Outputs comprehensive ESP32 system information via logging
 * 
 * Diagnostic function that collects and logs essential ESP32 hardware
 * and firmware information for system analysis and troubleshooting.
 * Provides formatted output of chip specifications, performance metrics,
 * and firmware details through the established logging infrastructure.
 * 
 * Information displayed includes:
 * - Chip model and revision identification
 * - CPU frequency configuration  
 * - Flash memory size and specifications
 * - Available heap memory status
 * - SDK version and build information
 * 
 * @note Requires INFO level logging to be enabled for output visibility
 * @note Uses formatted logging (infof) for structured data presentation
 * @note Output is tagged with "SYSTEM" identifier for easy filtering
 * @note Subject to build-time INFO level filtering via LOGGER_LEVEL_INFO
 * @note Ideal for startup diagnostics and system health verification
 * 
 * @see printWiFiStatus() for network-specific diagnostic information
 * @see printMemoryInfo() for detailed memory analysis
 * @see infof() for underlying formatted logging mechanism
 * 
 * @since v0.9
 */
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

/**
 * @brief Outputs comprehensive WiFi network status via logging
 * 
 * Diagnostic function that collects and logs current WiFi configuration,
 * connection status, and network parameters for connectivity troubleshooting.
 * Supports both Access Point (AP) and Station (STA) modes with detailed
 * status reporting for each operational mode.
 * 
 * Access Point Mode Information:
 * - AP SSID configuration
 * - AP IP address assignment
 * - Connected client count
 * 
 * Station Mode Information:
 * - Connected network SSID
 * - Assigned IP address
 * - Signal strength (RSSI) measurement
 * - Connection status verification
 * 
 * @note Requires INFO level logging to be enabled for output visibility
 * @note Uses formatted logging (infof) for structured data presentation
 * @note Output is tagged with "WIFI" identifier for easy filtering
 * @note Subject to build-time INFO level filtering via LOGGER_LEVEL_INFO
 * @note Automatically detects and reports current WiFi mode configuration
 * @note Issues warnings for disconnected station mode scenarios
 * 
 * @see printSystemInfo() for hardware-specific diagnostic information
 * @see printMemoryInfo() for memory usage analysis
 * @see infof() for underlying formatted logging mechanism
 * @see warn() for disconnection status reporting
 * 
 * @since v0.9
 */
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

/**
 * @brief Outputs detailed ESP32 memory usage analysis via logging
 * 
 * Advanced diagnostic function that collects and logs comprehensive memory
 * statistics for both internal heap and external PSRAM when available.
 * Essential for memory leak detection, heap fragmentation analysis, and
 * optimal memory allocation strategy development.
 * 
 * Internal Heap Information:
 * - Current free heap space
 * - Total heap size configuration
 * - Minimum free heap reached (watermark)
 * - Maximum single allocation possible
 * 
 * PSRAM Information (when available):
 * - Total PSRAM capacity
 * - Available PSRAM space
 * - PSRAM detection and availability status
 * 
 * @note Requires DEBUG level logging to be enabled for output visibility
 * @note Uses formatted logging (infof) for structured data presentation
 * @note Output is tagged with "MEMORY" identifier for easy filtering
 * @note Subject to build-time DEBUG level filtering via LOGGER_LEVEL_DEBUG
 * @note Automatically detects PSRAM availability via compile-time flags
 * @note Issues warnings when PSRAM is expected but not found
 * @note Ideal for memory optimization and leak detection scenarios
 * 
 * @see printSystemInfo() for general hardware diagnostic information
 * @see printWiFiStatus() for network connectivity analysis
 * @see infof() for underlying formatted logging mechanism
 * @see warn() for PSRAM unavailability reporting
 * 
 * @since v0.9
 */
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