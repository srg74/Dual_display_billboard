#pragma once

// Build Information (auto-generated during compilation)
#ifndef BUILD_DATE
#define BUILD_DATE "2508174"  // YYMMDDx format x = daily build counter
#endif

#ifndef BUILD_TYPE
#ifdef LOGGER_ENABLED
#if LOGGER_ENABLED == 1
#define BUILD_TYPE "debug"
#else
#define BUILD_TYPE "production"
#endif
#endif
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "v0.9"
#endif

// Network Configuration
#define PORTAL_SSID "Billboard-Portal"
#define AP_MAX_CONNECTIONS 4
#define CONNECTION_TIMEOUT 10000     // 10 seconds
#define WIFI_RETRY_DELAY 5000       // 5 seconds

// GPIO Configuration
#define GPIO0_PIN 0
#define FACTORY_RESET_DURATION 6000  // 6 seconds

// Display Configuration
#define DISPLAY_BRIGHTNESS_DEFAULT 255
#define DISPLAY_ROTATION_DEFAULT 0
#define BACKLIGHT_PWM_CHANNEL 1
#define BACKLIGHT_PWM_FREQUENCY 5000
#define BACKLIGHT_PWM_RESOLUTION 8

// Timing Configuration
#define HEARTBEAT_INTERVAL 10000
#define DISPLAY_TEST_INTERVAL 5000
#define STARTUP_DELAY 2000

// Memory Management
#define LOW_MEMORY_THRESHOLD 50000   // 50KB
#define CRITICAL_MEMORY_THRESHOLD 20000  // 20KB

// Web Server Configuration
#define WEB_SERVER_PORT 80
#define WEBSOCKET_PORT 81
#define MAX_UPLOAD_SIZE (2 * 1024 * 1024)  // 2MB

// File System Configuration
#define MAX_FILENAME_LENGTH 64
#define MAX_FILES_IN_DIRECTORY 50

// Time Configuration
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define NTP_SERVER3 "time.google.com"
#define DEFAULT_TIMEZONE "UTC0"
#define TIME_SYNC_INTERVAL 3600000  // 1 hour in milliseconds