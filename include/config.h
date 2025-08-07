#pragma once

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
#define DISPLAY_ROTATION_DEFAULT 3
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