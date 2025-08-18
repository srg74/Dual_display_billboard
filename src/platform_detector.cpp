/**
 * @file platform_detector.cpp
 * @brief Multiplatform detection and PSRAM capability testing implementation
 * 
 * Provides comprehensive hardware platform detection across the ESP32 family
 * with specialized PSRAM capability testing and board variant identification.
 * Essential for multiplatform compatibility and optimal hardware utilization.
 * 
 * Supports automatic detection of:
 * - ESP32 Classic (original) variants
 * - ESP32-S2 single-core variants  
 * - ESP32-S3 dual-core with PSRAM variants (N8R2, N16R8)
 * - ESP32-C3 RISC-V variants
 * - Custom and unknown hardware configurations
 * 
 * PSRAM detection includes both compile-time configuration validation
 * and runtime hardware presence verification with functional testing.
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 0.9
 * @since v0.9
 */

#include "platform_detector.h"
#include "logger.h"
#include <esp_chip_info.h>
#include <esp_heap_caps.h>

const char* PlatformDetector::TAG = "PLATFORM";

/**
 * @brief Performs comprehensive ESP32 family platform detection and analysis
 * 
 * Core detection function that identifies hardware platform characteristics
 * including chip model, CPU configuration, memory specifications, and PSRAM
 * availability. Provides complete hardware profile for optimal system configuration.
 * 
 * Detection Process:
 * 1. ESP chip information retrieval via esp_chip_info()
 * 2. Chip model classification (ESP32/S2/S3/C3)
 * 3. CPU core count and frequency detection
 * 4. Flash memory size determination
 * 5. PSRAM configuration and presence validation
 * 6. Board variant identification based on memory configuration
 * 7. Heap memory statistics collection
 * 
 * Board Variant Detection:
 * - ESP32 Classic: DevKit (No PSRAM)
 * - ESP32-S3 N8: Standard without PSRAM
 * - ESP32-S3 N8R2: 2MB PSRAM variant
 * - ESP32-S3 N16R8: 8MB PSRAM variant
 * - Custom configurations: Size-based classification
 * 
 * @return PlatformInfo Complete hardware profile structure
 * 
 * @note Uses compile-time BOARD_HAS_PSRAM flag for PSRAM configuration detection
 * @note Distinguishes between PSRAM compilation support and actual hardware presence
 * @note Board variant identification assists with optimal memory allocation strategies
 * @note CPU frequency and core count enable platform-specific performance tuning
 * 
 * @see printPlatformInfo() for formatted output of detection results
 * @see testPSRAMAllocation() for functional PSRAM validation
 * @see getPlatformSummary() for concise platform description
 * 
 * @since v0.9
 */
PlatformDetector::PlatformInfo PlatformDetector::detectPlatform() {
    PlatformInfo info = {};
    
    // Get chip information
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    // Detect chip model
    switch(chip_info.model) {
        case CHIP_ESP32:
            info.chipModel = CHIP_ESP32_CLASSIC;
            strcpy(info.chipName, "ESP32");
            info.expectedPSRAM = false;
            break;
        case CHIP_ESP32S2:
            info.chipModel = CHIP_ESP32_S2;
            strcpy(info.chipName, "ESP32-S2");
            info.expectedPSRAM = false;
            break;
        case CHIP_ESP32S3:
            info.chipModel = CHIP_ESP32_S3;
            strcpy(info.chipName, "ESP32-S3");
            info.expectedPSRAM = true;  // Most S3 variants have PSRAM
            break;
        case CHIP_ESP32C3:
            info.chipModel = CHIP_ESP32_C3;
            strcpy(info.chipName, "ESP32-C3");
            info.expectedPSRAM = false;
            break;
        default:
            info.chipModel = CHIP_UNKNOWN;
            strcpy(info.chipName, "Unknown");
            info.expectedPSRAM = false;
            break;
    }
    
    // Detect CPU cores
    info.cpuCores = chip_info.cores;
    
    // Detect CPU frequency
    info.cpuFreqMHz = ESP.getCpuFreqMHz();
    
    // Detect flash size
    info.flashSize = ESP.getFlashChipSize();
    
    // Detect PSRAM availability
    #ifdef BOARD_HAS_PSRAM
    info.psramConfigured = true;
    if (psramFound()) {
        info.psramActuallyPresent = true;
        info.psramSize = ESP.getPsramSize();
        info.psramFree = ESP.getFreePsram();
        
        // Try to determine PSRAM type based on size
        if (info.psramSize >= 8 * 1024 * 1024) {
            strcpy(info.boardVariant, "N16R8 (8MB PSRAM)");
        } else if (info.psramSize >= 2 * 1024 * 1024) {
            strcpy(info.boardVariant, "N8R2 (2MB PSRAM)");
        } else {
            strcpy(info.boardVariant, "Custom PSRAM");
        }
    } else {
        info.psramActuallyPresent = false;
        info.psramSize = 0;
        info.psramFree = 0;
        strcpy(info.boardVariant, "N8 (No PSRAM)");
    }
    #else
    info.psramConfigured = false;
    info.psramActuallyPresent = false;
    info.psramSize = 0;
    info.psramFree = 0;
    if (info.chipModel == CHIP_ESP32_CLASSIC) {
        strcpy(info.boardVariant, "DevKit (No PSRAM)");
    } else {
        strcpy(info.boardVariant, "Unknown");
    }
    #endif
    
    // Detect heap information
    info.heapSize = ESP.getHeapSize();
    info.heapFree = ESP.getFreeHeap();
    
    return info;
}

/**
 * @brief Outputs comprehensive platform detection results via logging system
 * 
 * Formatted display function that presents complete hardware platform analysis
 * through the centralized logging infrastructure. Provides structured output
 * ideal for system diagnostics, development debugging, and user support.
 * 
 * Output Information Categories:
 * - Chip identification (model, cores, frequency)
 * - Board variant classification and memory configuration
 * - Flash memory capacity and heap statistics
 * - PSRAM configuration status and functional verification
 * - Platform compatibility assessment and optimization recommendations
 * 
 * PSRAM Status Reporting:
 * - CONFIGURED BUT NOT DETECTED: Hardware mismatch indication
 * - DETECTED AND FUNCTIONAL: Optimal configuration confirmation
 * - NOT CONFIGURED: ESP32 Classic mode identification
 * 
 * Compatibility Assessment:
 * - OPTIMAL: Hardware matches chip expectations (S3 with PSRAM, ESP32 without)
 * - SUBOPTIMAL: Configuration mismatch requiring attention
 * 
 * @param info Platform information structure from detectPlatform()
 * 
 * @note Requires INFO level logging for standard output visibility
 * @note Uses WARNING level for hardware mismatch conditions
 * @note All output tagged with "PLATFORM" for easy log filtering
 * @note Memory sizes displayed in user-friendly MB/KB units
 * @note Compatibility status guides optimal system configuration
 * 
 * @see detectPlatform() for platform information collection
 * @see Logger::infof() for formatted logging output mechanism
 * @see Logger::warnf() for hardware mismatch warnings
 * 
 * @since v0.9
 */
void PlatformDetector::printPlatformInfo(const PlatformInfo& info) {
    LOG_INFOF(TAG, "=== MULTIPLATFORM DETECTION RESULTS ===");
    LOG_INFOF(TAG, "Chip: %s (%d cores @ %d MHz)", info.chipName, info.cpuCores, info.cpuFreqMHz);
    LOG_INFOF(TAG, "Board Variant: %s", info.boardVariant);
    LOG_INFOF(TAG, "Flash: %.1f MB", info.flashSize / (1024.0 * 1024.0));
    LOG_INFOF(TAG, "Heap: %.1f KB total, %.1f KB free", 
              info.heapSize / 1024.0, info.heapFree / 1024.0);
    
    LOG_INFOF(TAG, "PSRAM Configuration: %s", info.psramConfigured ? "ENABLED" : "DISABLED");
    if (info.psramConfigured) {
        if (info.psramActuallyPresent) {
            LOG_INFOF(TAG, "PSRAM Status: DETECTED AND FUNCTIONAL");
            LOG_INFOF(TAG, "PSRAM: %.1f MB total, %.1f MB free", 
                      info.psramSize / (1024.0 * 1024.0), info.psramFree / (1024.0 * 1024.0));
        } else {
            LOG_WARNF(TAG, "PSRAM Status: CONFIGURED BUT NOT DETECTED");
            LOG_WARNF(TAG, "This may indicate hardware mismatch");
        }
    } else {
        LOG_INFOF(TAG, "PSRAM Status: NOT CONFIGURED (ESP32 Classic Mode)");
    }
    
    LOG_INFOF(TAG, "Platform Compatibility: %s", 
              (info.expectedPSRAM == info.psramActuallyPresent) ? "OPTIMAL" : "SUBOPTIMAL");
    LOG_INFOF(TAG, "=====================================");
}

/**
 * @brief Performs comprehensive PSRAM functionality testing and validation
 * 
 * Advanced testing function that validates PSRAM hardware functionality through
 * systematic allocation tests across multiple memory sizes. Essential for
 * confirming PSRAM reliability before production deployment and memory-intensive operations.
 * 
 * Test Methodology:
 * 1. PSRAM presence verification via psramFound()
 * 2. Progressive allocation testing (1KB → 256KB)
 * 3. Memory write pattern validation (0xAA test pattern)
 * 4. Boundary condition verification (first/last byte integrity)
 * 5. Proper memory deallocation and cleanup
 * 6. Success rate calculation and reporting
 * 
 * Test Allocation Sizes:
 * - 1KB: Small allocation baseline
 * - 4KB: Page-size allocation testing
 * - 16KB: Medium buffer validation
 * - 64KB: Large buffer capability
 * - 256KB: Maximum typical allocation test
 * 
 * @return true if all PSRAM tests pass or not applicable for platform
 * @return false if PSRAM configured but tests fail
 * 
 * @note Only executes when BOARD_HAS_PSRAM compile flag is enabled
 * @note Uses heap_caps_malloc() with MALLOC_CAP_SPIRAM for PSRAM-specific allocation
 * @note Test pattern 0xAA provides reliable write/read verification
 * @note Boundary testing ensures full allocation integrity
 * @note All test results logged with detailed size and status information
 * @note Non-PSRAM platforms return true (not considered an error condition)
 * 
 * @see detectPlatform() for PSRAM configuration detection
 * @see printPlatformInfo() for PSRAM status reporting
 * @see heap_caps_malloc() for PSRAM-specific memory allocation
 * 
 * @since v0.9
 */
bool PlatformDetector::testPSRAMAllocation() {
    #ifdef BOARD_HAS_PSRAM
    if (!psramFound()) {
        LOG_WARNF(TAG, "PSRAM allocation test skipped - no PSRAM detected");
        return false;
    }
    
    LOG_INFOF(TAG, "Testing PSRAM allocation capabilities...");
    
    // Test different allocation sizes
    const size_t testSizes[] = {1024, 4096, 16384, 65536, 262144}; // 1KB to 256KB
    const int numTests = sizeof(testSizes) / sizeof(testSizes[0]);
    int successCount = 0;
    
    for (int i = 0; i < numTests; i++) {
        void* psramPtr = heap_caps_malloc(testSizes[i], MALLOC_CAP_SPIRAM);
        if (psramPtr != nullptr) {
            // Test write/read
            memset(psramPtr, 0xAA, testSizes[i]);
            bool writeOk = (((uint8_t*)psramPtr)[0] == 0xAA);
            bool writeOk2 = (((uint8_t*)psramPtr)[testSizes[i]-1] == 0xAA);
            
            if (writeOk && writeOk2) {
                LOG_INFOF(TAG, "   PSRAM allocation test %d KB: SUCCESS", testSizes[i] / 1024);
                successCount++;
            } else {
                LOG_ERRORF(TAG, "   PSRAM allocation test %d KB: WRITE FAILED", testSizes[i] / 1024);
            }
            
            heap_caps_free(psramPtr);
        } else {
            LOG_ERRORF(TAG, "   PSRAM allocation test %d KB: ALLOCATION FAILED", testSizes[i] / 1024);
        }
    }
    
    LOG_INFOF(TAG, "PSRAM allocation test results: %d/%d successful", successCount, numTests);
    return (successCount == numTests);
    #else
    LOG_INFOF(TAG, "PSRAM allocation test skipped - not configured for this platform");
    return true; // Not an error for non-PSRAM platforms
    #endif
}

/**
 * @brief Generates concise platform summary string for web interfaces and displays
 * 
 * Utility function that creates compact, user-friendly platform identification
 * suitable for web interfaces, status displays, and API responses. Provides
 * essential hardware information in minimal space while maintaining clarity.
 * 
 * Summary Format Examples:
 * - "Platform: ESP32 DevKit (No PSRAM) | No PSRAM"
 * - "Platform: ESP32-S3 N8R2 (2MB PSRAM) | PSRAM: 2MB"
 * - "Platform: ESP32-S3 N16R8 (8MB PSRAM) | PSRAM: 8MB"
 * - "Platform: ESP32-C3 Unknown | No PSRAM"
 * 
 * Information Components:
 * - Chip name identification (ESP32, ESP32-S3, etc.)
 * - Board variant classification (N8, N8R2, N16R8, DevKit)
 * - PSRAM status and capacity (if present)
 * - Compact formatting suitable for constrained display space
 * 
 * @return String Formatted platform summary for display purposes
 * 
 * @note Uses detectPlatform() internally for current hardware analysis
 * @note PSRAM capacity displayed in MB units for readability
 * @note String format optimized for web UI status displays
 * @note Consistent formatting across all supported platforms
 * @note Suitable for JSON API responses and embedded displays
 * 
 * @see detectPlatform() for comprehensive platform information
 * @see printPlatformInfo() for detailed diagnostic output
 * @see PlatformInfo for complete hardware specification structure
 * 
 * @since v0.9
 */
String PlatformDetector::getPlatformSummary() {
    PlatformInfo info = detectPlatform();
    
    String summary = "Platform: " + String(info.chipName) + " " + String(info.boardVariant);
    
    if (info.psramActuallyPresent) {
        summary += " | PSRAM: " + String(info.psramSize / (1024 * 1024)) + "MB";
    } else {
        summary += " | No PSRAM";
    }
    
    return summary;
}
