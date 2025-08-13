/**
 * @file platform_detector.h
 * @brief Multiplatform detection and PSRAM capability testing
 * 
 * Provides comprehensive platform detection for ESP32 family chips
 * with focus on PSRAM availability and multiplatform compatibility.
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <Arduino.h>
#include <esp_chip_info.h>

/**
 * @brief Platform detection and PSRAM testing utility
 * 
 * This class provides comprehensive detection of ESP32 family hardware
 * variants and their PSRAM capabilities, ensuring proper multiplatform
 * support across ESP32, ESP32-S2, ESP32-S3, and ESP32-C3 variants.
 */
class PlatformDetector {
public:
    /**
     * @brief Supported chip models
     */
    enum ChipModel {
        CHIP_ESP32_CLASSIC = 0,  ///< Original ESP32
        CHIP_ESP32_S2 = 1,       ///< ESP32-S2
        CHIP_ESP32_S3 = 2,       ///< ESP32-S3 (with PSRAM variants)
        CHIP_ESP32_C3 = 3,       ///< ESP32-C3
        CHIP_UNKNOWN = 99        ///< Unknown or unsupported chip
    };
    
    /**
     * @brief Comprehensive platform information structure
     */
    struct PlatformInfo {
        ChipModel chipModel;          ///< Detected chip model
        char chipName[32];            ///< Human-readable chip name
        char boardVariant[64];        ///< Board variant (N8, N8R2, N16R8, etc.)
        
        uint8_t cpuCores;             ///< Number of CPU cores
        uint16_t cpuFreqMHz;          ///< CPU frequency in MHz
        size_t flashSize;             ///< Flash memory size in bytes
        
        size_t heapSize;              ///< Total heap size
        size_t heapFree;              ///< Free heap memory
        
        bool psramConfigured;         ///< PSRAM support compiled in
        bool psramActuallyPresent;    ///< PSRAM hardware detected
        bool expectedPSRAM;           ///< Expected PSRAM for this chip
        size_t psramSize;             ///< Total PSRAM size (if present)
        size_t psramFree;             ///< Free PSRAM memory (if present)
    };
    
    /**
     * @brief Detect current platform and capabilities
     * @return Complete platform information structure
     */
    static PlatformInfo detectPlatform();
    
    /**
     * @brief Print comprehensive platform information to log
     * @param info Platform information to display
     */
    static void printPlatformInfo(const PlatformInfo& info);
    
    /**
     * @brief Test PSRAM allocation and functionality
     * @return true if PSRAM tests pass (or not applicable)
     */
    static bool testPSRAMAllocation();
    
    /**
     * @brief Get concise platform summary string
     * @return Brief platform description for web interfaces
     */
    static String getPlatformSummary();

private:
    static const char* TAG;           ///< Logging tag
};

// Convenience macros for multiplatform compatibility
#ifdef ESP32S3_MODE
    #define PLATFORM_HAS_PSRAM_SUPPORT true
    #define PLATFORM_NAME "ESP32-S3"
#else
    #define PLATFORM_HAS_PSRAM_SUPPORT false
    #define PLATFORM_NAME "ESP32"
#endif

// PSRAM allocation helpers that work on all platforms
#ifdef BOARD_HAS_PSRAM
    #define PSRAM_MALLOC(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
    #define PSRAM_CALLOC(count, size) heap_caps_calloc(count, size, MALLOC_CAP_SPIRAM)
    #define PSRAM_REALLOC(ptr, size) heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM)
    #define PSRAM_FREE(ptr) heap_caps_free(ptr)
    #define PSRAM_AVAILABLE() psramFound()
#else
    #define PSRAM_MALLOC(size) malloc(size)
    #define PSRAM_CALLOC(count, size) calloc(count, size)
    #define PSRAM_REALLOC(ptr, size) realloc(ptr, size)
    #define PSRAM_FREE(ptr) free(ptr)
    #define PSRAM_AVAILABLE() false
#endif
