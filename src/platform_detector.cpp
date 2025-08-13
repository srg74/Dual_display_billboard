/**
 * @file platform_detector.cpp
 * @brief Multiplatform detection and PSRAM capability testing
 * 
 * Demonstrates ESP32 vs ESP32-S3 detection and PSRAM availability
 * across different hardware variants.
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 1.0.0
 */

#include "platform_detector.h"
#include "logger.h"
#include <esp_chip_info.h>
#include <esp_heap_caps.h>

const char* PlatformDetector::TAG = "PLATFORM";

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
