/**
 * @file logger.h
 * @brief Hierarchical logging system with build-time optimization for ESP32 dual display billboard
 * 
 * Provides comprehensive logging infrastructure with five priority levels and build-time
 * filtering for optimal performance. Designed for embedded systems requiring both
 * development debugging capabilities and production performance optimization.
 * 
 * Key Features:
 * - Five-level priority hierarchy (ERROR → WARN → INFO → DEBUG → VERBOSE)
 * - Build-time level filtering via preprocessor flags for zero runtime overhead
 * - Printf-style formatted logging with variable arguments support
 * - Convenience wrapper functions for simplified usage
 * - System diagnostic functions for hardware and network status
 * - Professional text-only output formatting without visual clutter
 * - Singleton pattern with lazy initialization for resource efficiency
 * 
 * Build-Time Optimization:
 * - LOGGER_LEVEL_ERROR: Critical errors only
 * - LOGGER_LEVEL_WARN: Warnings and above
 * - LOGGER_LEVEL_INFO: General information and above (production default)
 * - LOGGER_LEVEL_DEBUG: Development debugging and above
 * - LOGGER_LEVEL_VERBOSE: Maximum detail for intensive debugging
 * 
 * Usage Examples:
 * ```cpp
 * Logger::info("STARTUP", "System initialized successfully");
 * Logger::errorf("WIFI", "Connection failed: %s", error_message);
 * Logger::printSystemInfo(); // Hardware diagnostics
 * ```
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 0.9
 * @since v0.9
 */

#pragma once
#include <Arduino.h>

// Use build flags directly - don't redefine them
// The build flags from platformio.ini will be used automatically

/**
 * @brief Hierarchical logging system with build-time optimization
 * 
 * Singleton logger class providing comprehensive logging infrastructure for
 * embedded ESP32 systems. Features five priority levels with build-time
 * filtering to eliminate runtime overhead in production environments.
 * 
 * The logger automatically initializes on first use and provides both
 * simple string logging and printf-style formatted output. All logging
 * calls are subject to compile-time filtering based on build flags,
 * ensuring zero performance impact for disabled log levels.
 * 
 * Thread Safety: Not required for single-threaded ESP32 Arduino environment
 * Memory Usage: Minimal static allocation with 512-byte formatting buffers
 * Performance: Zero overhead for disabled levels via preprocessor elimination
 */
class Logger {
private:
    static bool initialized;       ///< Lazy initialization state tracking
    
    /**
     * @brief Ensures logger is initialized before use
     * 
     * Lazy initialization function called automatically by all public
     * logging methods. Initializes Serial communication and system
     * state on first invocation only.
     * 
     * @note Thread-safe not required in single-threaded Arduino environment
     * @note Called automatically - no need for manual invocation
     * @see init() for explicit initialization with custom baud rate
     */
    static void ensureInitialized();
    
public:
    /**
     * @brief Logging priority levels in ascending order of verbosity
     * 
     * Five-level hierarchy designed for embedded systems with clear
     * separation between production-suitable levels (ERROR/WARN/INFO)
     * and development levels (DEBUG/VERBOSE).
     */
    enum Level {
        ERROR = 1,    ///< Critical errors requiring immediate attention
        WARN = 2,     ///< Non-critical issues and recoverable errors
        INFO = 3,     ///< General operational information (production default)
        DEBUG = 4,    ///< Development debugging and detailed state information
        VERBOSE = 5   ///< Maximum detail for intensive debugging scenarios
    };
    
    /**
     * @brief Explicit logger initialization with custom baud rate
     * @param baudRate Serial communication speed (default: 115200)
     * @see ensureInitialized() for automatic initialization
     */
    static void init(unsigned long baudRate = 115200);
    
    /**
     * @brief Sets runtime log level threshold
     * @param level Minimum level for log output
     * @note Build-time filtering takes precedence over runtime level
     */
    static void setLevel(Level level);
    
    /**
     * @brief Gets current runtime log level threshold
     * @return Current minimum log level
     */
    static Level getLevel();
    
    // Core logging functions
    
    /**
     * @brief Core logging function with explicit level specification
     * @param level Log level for this message
     * @param tag Component identifier for message categorization
     * @param message Log message content
     * @note Subject to both build-time and runtime level filtering
     */
    static void log(Level level, const String& tag, const String& message);
    
    /**
     * @brief Printf-style formatted logging with explicit level
     * @param level Log level for this message
     * @param tag Component identifier for message categorization  
     * @param format Printf-style format string
     * @param ... Variable arguments for format specifiers
     * @note Uses 512-byte internal buffer for message formatting
     */
    static void logf(Level level, const String& tag, const char* format, ...);
    
    // Convenience functions
    
    /**
     * @brief Log error-level message (highest priority)
     * @param tag Component identifier
     * @param message Error message content
     */
    static void error(const String& tag, const String& message);
    
    /**
     * @brief Log warning-level message  
     * @param tag Component identifier
     * @param message Warning message content
     */
    static void warn(const String& tag, const String& message);
    
    /**
     * @brief Log informational message (production default)
     * @param tag Component identifier
     * @param message Information content
     */
    static void info(const String& tag, const String& message);
    
    /**
     * @brief Log debug-level message (development use)
     * @param tag Component identifier
     * @param message Debug information
     */
    static void debug(const String& tag, const String& message);
    
    /**
     * @brief Log verbose-level message (maximum detail)
     * @param tag Component identifier
     * @param message Verbose debugging content
     */
    static void verbose(const String& tag, const String& message);
    
    // Printf-style functions
    
    /**
     * @brief Printf-style error logging
     * @param tag Component identifier
     * @param format Printf format string
     * @param ... Variable arguments
     */
    static void errorf(const String& tag, const char* format, ...);
    
    /**
     * @brief Printf-style warning logging
     * @param tag Component identifier
     * @param format Printf format string
     * @param ... Variable arguments
     */
    static void warnf(const String& tag, const char* format, ...);
    
    /**
     * @brief Printf-style informational logging
     * @param tag Component identifier
     * @param format Printf format string
     * @param ... Variable arguments
     */
    static void infof(const String& tag, const char* format, ...);
    
    /**
     * @brief Printf-style debug logging
     * @param tag Component identifier
     * @param format Printf format string
     * @param ... Variable arguments
     */
    static void debugf(const String& tag, const char* format, ...);
    
    /**
     * @brief Printf-style verbose logging
     * @param tag Component identifier  
     * @param format Printf format string
     * @param ... Variable arguments
     */
    static void verbosef(const String& tag, const char* format, ...);
    
    // System info logging
    
    /**
     * @brief Output comprehensive ESP32 system information
     * @note Displays chip model, CPU freq, memory, SDK version
     */
    static void printSystemInfo();
    
    /**
     * @brief Output detailed WiFi connection status
     * @note Shows AP/STA modes, connection status, signal strength
     */
    static void printWiFiStatus();
    
    /**
     * @brief Output memory usage analysis including PSRAM
     * @note Requires DEBUG level logging for visibility
     */
    static void printMemoryInfo();
};

// Convenience macros for easy logging - use build flags directly
#if defined(LOGGER_ENABLED) && LOGGER_ENABLED

#define LOG_ERROR(tag, msg) do { if (LOGGER_LEVEL_ERROR) Logger::error(tag, msg); } while(0)
#define LOG_WARN(tag, msg) do { if (LOGGER_LEVEL_WARN) Logger::warn(tag, msg); } while(0)
#define LOG_INFO(tag, msg) do { if (LOGGER_LEVEL_INFO) Logger::info(tag, msg); } while(0)
#define LOG_DEBUG(tag, msg) do { if (LOGGER_LEVEL_DEBUG) Logger::debug(tag, msg); } while(0)
#define LOG_VERBOSE(tag, msg) do { if (LOGGER_LEVEL_VERBOSE) Logger::verbose(tag, msg); } while(0)

#define LOG_ERRORF(tag, fmt, ...) do { if (LOGGER_LEVEL_ERROR) Logger::errorf(tag, fmt, ##__VA_ARGS__); } while(0)
#define LOG_WARNF(tag, fmt, ...) do { if (LOGGER_LEVEL_WARN) Logger::warnf(tag, fmt, ##__VA_ARGS__); } while(0)
#define LOG_INFOF(tag, fmt, ...) do { if (LOGGER_LEVEL_INFO) Logger::infof(tag, fmt, ##__VA_ARGS__); } while(0)
#define LOG_DEBUGF(tag, fmt, ...) do { if (LOGGER_LEVEL_DEBUG) Logger::debugf(tag, fmt, ##__VA_ARGS__); } while(0)
#define LOG_VERBOSEF(tag, fmt, ...) do { if (LOGGER_LEVEL_VERBOSE) Logger::verbosef(tag, fmt, ##__VA_ARGS__); } while(0)

// System logging macros
#define LOG_SYSTEM_INFO() do { if (LOGGER_LEVEL_INFO) Logger::printSystemInfo(); } while(0)
#define LOG_WIFI_STATUS() do { if (LOGGER_LEVEL_INFO) Logger::printWiFiStatus(); } while(0)
#define LOG_MEMORY_INFO() do { if (LOGGER_LEVEL_DEBUG) Logger::printMemoryInfo(); } while(0)

#else

// Disabled logging - all macros become no-ops
#define LOG_ERROR(tag, msg) do {} while(0)
#define LOG_WARN(tag, msg) do {} while(0)
#define LOG_INFO(tag, msg) do {} while(0)
#define LOG_DEBUG(tag, msg) do {} while(0)
#define LOG_VERBOSE(tag, msg) do {} while(0)

#define LOG_ERRORF(tag, fmt, ...) do {} while(0)
#define LOG_WARNF(tag, fmt, ...) do {} while(0)
#define LOG_INFOF(tag, fmt, ...) do {} while(0)
#define LOG_DEBUGF(tag, fmt, ...) do {} while(0)
#define LOG_VERBOSEF(tag, fmt, ...) do {} while(0)

#define LOG_SYSTEM_INFO() do {} while(0)
#define LOG_WIFI_STATUS() do {} while(0)
#define LOG_MEMORY_INFO() do {} while(0)

#endif