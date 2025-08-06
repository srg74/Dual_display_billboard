#pragma once
#include <Arduino.h>

// Use build flags directly - don't redefine them
// The build flags from platformio.ini will be used automatically

// Logger class
class Logger {
private:
    static bool initialized;
    static void ensureInitialized();
    
public:
    enum Level {
        ERROR = 1,
        WARN = 2,
        INFO = 3,
        DEBUG = 4,
        VERBOSE = 5
    };
    
    static void init(unsigned long baudRate = 115200);
    static void setLevel(Level level);
    static Level getLevel();
    
    // Core logging functions
    static void log(Level level, const String& tag, const String& message);
    static void logf(Level level, const String& tag, const char* format, ...);
    
    // Convenience functions
    static void error(const String& tag, const String& message);
    static void warn(const String& tag, const String& message);
    static void info(const String& tag, const String& message);
    static void debug(const String& tag, const String& message);
    static void verbose(const String& tag, const String& message);
    
    // Printf-style functions
    static void errorf(const String& tag, const char* format, ...);
    static void warnf(const String& tag, const char* format, ...);
    static void infof(const String& tag, const char* format, ...);
    static void debugf(const String& tag, const char* format, ...);
    static void verbosef(const String& tag, const char* format, ...);
    
    // System info logging
    static void printSystemInfo();
    static void printWiFiStatus();
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