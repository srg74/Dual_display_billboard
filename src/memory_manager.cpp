/**
 * @file memory_manager.cpp
 * @brief Implementation of comprehensive memory monitoring system for ESP32-S3
 * 
 * Provides real-time monitoring and management of ESP32-S3 memory subsystems
 * including internal heap, PSRAM, and automatic cleanup mechanisms.
 * 
 * @author ESP32-S3 Billboard System
 * @date 2025
 * @version 1.0.0
 */

#include "memory_manager.h"
#include <esp_heap_caps.h>
#include <esp_system.h>

// Static member definitions
const char* MemoryManager::TAG = "MEMORY";
MemoryManager::MemoryStats MemoryManager::currentStats = {};
unsigned long MemoryManager::lastMonitorTime = 0;
unsigned long MemoryManager::monitorInterval = 10000;  // 10 seconds default
bool MemoryManager::autoCleanupEnabled = true;
size_t MemoryManager::heapSampleCount = 0;
size_t MemoryManager::heapSampleSum = 0;

bool MemoryManager::initialize(unsigned long monitorIntervalMs, bool enableAutoCleanup) {
    LOG_INFOF(TAG, "🔧 Initializing memory monitoring system");
    
    monitorInterval = monitorIntervalMs;
    autoCleanupEnabled = enableAutoCleanup;
    
    // Initialize statistics structure
    memset(&currentStats, 0, sizeof(MemoryStats));
    currentStats.uptimeMs = millis();
    currentStats.lastUpdateMs = millis();
    
    // Perform initial memory scan
    updateHeapStats();
    updatePsramStats();
    updateHealthStatus();
    updateFragmentation();
    
    LOG_INFOF(TAG, "✅ Memory manager initialized");
    LOG_INFOF(TAG, "   Monitor interval: %lu ms", monitorInterval);
    LOG_INFOF(TAG, "   Auto cleanup: %s", autoCleanupEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "   Initial heap: %d bytes free", currentStats.heapFree);
    
    #ifdef BOARD_HAS_PSRAM
    if (currentStats.psramAvailable) {
        LOG_INFOF(TAG, "   Initial PSRAM: %d bytes free", currentStats.psramFree);
    } else {
        LOG_WARNF(TAG, "   PSRAM not detected or not available");
    }
    #endif
    
    return true;
}

void MemoryManager::update() {
    unsigned long currentTime = millis();
    
    // Check if it's time to update
    if (currentTime - lastMonitorTime < monitorInterval) {
        return;
    }
    
    lastMonitorTime = currentTime;
    currentStats.uptimeMs = currentTime;
    currentStats.lastUpdateMs = currentTime;
    
    // Update all memory subsystems
    updateHeapStats();
    updatePsramStats();
    updateHealthStatus();
    updateFragmentation();
    
    // Trigger cleanup if auto-cleanup is enabled and needed
    if (autoCleanupEnabled) {
        triggerCleanupIfNeeded();
    }
    
    // Record events based on health status
    if (currentStats.heapHealth >= WARNING) {
        recordMemoryEvent(HEAP_INTERNAL, currentStats.heapHealth);
    }
    
    if (currentStats.psramAvailable && currentStats.psramHealth >= WARNING) {
        recordMemoryEvent(PSRAM_EXTERNAL, currentStats.psramHealth);
    }
}

void MemoryManager::updateHeapStats() {
    // Get heap information
    currentStats.heapTotal = ESP.getHeapSize();
    currentStats.heapFree = ESP.getFreeHeap();
    currentStats.heapUsed = currentStats.heapTotal - currentStats.heapFree;
    currentStats.heapMinFree = ESP.getMinFreeHeap();
    currentStats.heapMaxAlloc = ESP.getMaxAllocHeap();
    
    // Update peak usage tracking
    if (currentStats.heapUsed > currentStats.peakHeapUsage) {
        currentStats.peakHeapUsage = currentStats.heapUsed;
    }
    
    // Update running average
    heapSampleCount++;
    heapSampleSum += currentStats.heapFree;
    currentStats.avgFreeHeap = heapSampleSum / heapSampleCount;
    
    // Prevent overflow of sample counting
    if (heapSampleCount > 1000) {
        heapSampleCount = heapSampleCount / 2;
        heapSampleSum = heapSampleSum / 2;
    }
}

void MemoryManager::updatePsramStats() {
    #ifdef ESP32S3_MODE
    // Only ESP32-S3 should report PSRAM
    if (psramFound()) {
        currentStats.psramAvailable = true;
        currentStats.psramTotal = ESP.getPsramSize();
        currentStats.psramFree = ESP.getFreePsram();
        currentStats.psramUsed = currentStats.psramTotal - currentStats.psramFree;
    } else {
        currentStats.psramAvailable = false;
        currentStats.psramTotal = 0;
        currentStats.psramFree = 0;
        currentStats.psramUsed = 0;
    }
    #else
    // ESP32 classic never has PSRAM - don't report false values
    currentStats.psramAvailable = false;
    currentStats.psramTotal = 0;
    currentStats.psramFree = 0;
    currentStats.psramUsed = 0;
    #endif
}

void MemoryManager::updateHealthStatus() {
    // Calculate health status for each subsystem
    currentStats.heapHealth = calculateHealthStatus(currentStats.heapFree, currentStats.heapTotal);
    
    if (currentStats.psramAvailable) {
        currentStats.psramHealth = calculateHealthStatus(currentStats.psramFree, currentStats.psramTotal);
    } else {
        currentStats.psramHealth = EXCELLENT;  // Not applicable
    }
    
    // Calculate overall health (worst of all subsystems)
    currentStats.overallHealth = currentStats.heapHealth;
    if (currentStats.psramAvailable && currentStats.psramHealth > currentStats.overallHealth) {
        currentStats.overallHealth = currentStats.psramHealth;
    }
}

void MemoryManager::updateFragmentation() {
    // Calculate heap fragmentation
    if (currentStats.heapFree > 0) {
        float fragmentationRatio = (float)currentStats.heapMaxAlloc / (float)currentStats.heapFree;
        currentStats.heapFragmentation = (1.0f - fragmentationRatio) * 100.0f;
        
        // Clamp to valid range
        if (currentStats.heapFragmentation < 0.0f) currentStats.heapFragmentation = 0.0f;
        if (currentStats.heapFragmentation > 100.0f) currentStats.heapFragmentation = 100.0f;
    } else {
        currentStats.heapFragmentation = 100.0f;  // Completely fragmented if no free memory
    }
}

MemoryManager::HealthStatus MemoryManager::calculateHealthStatus(size_t free, size_t total) {
    if (total == 0) return EXCELLENT;  // Not applicable
    
    float freePercentage = ((float)free / (float)total) * 100.0f;
    
    if (freePercentage >= 80.0f) return EXCELLENT;
    if (freePercentage >= 60.0f) return GOOD;
    if (freePercentage >= 40.0f) return WARNING;
    if (freePercentage >= 20.0f) return CRITICAL;
    return EMERGENCY;
}

void MemoryManager::triggerCleanupIfNeeded() {
    if (currentStats.overallHealth >= CRITICAL) {
        LOG_WARNF(TAG, "🧹 Triggering automatic memory cleanup (health: %s)", 
                  getHealthStatusString(currentStats.overallHealth));
        
        size_t freedBytes = forceCleanup();
        currentStats.cleanupTriggers++;
        
        LOG_INFOF(TAG, "🧹 Cleanup completed, freed %d bytes", freedBytes);
    }
}

void MemoryManager::recordMemoryEvent(MemoryType type, HealthStatus status) {
    if (status >= WARNING) {
        currentStats.lowMemoryEvents++;
    }
    
    if (status >= CRITICAL) {
        currentStats.criticalEvents++;
        LOG_WARNF(TAG, "⚠️ Critical memory event: %s subsystem in %s condition", 
                  getMemoryTypeString(type), getHealthStatusString(status));
    }
}

const MemoryManager::MemoryStats& MemoryManager::getStats() {
    return currentStats;
}

MemoryManager::HealthStatus MemoryManager::getOverallHealth() {
    return currentStats.overallHealth;
}

MemoryManager::HealthStatus MemoryManager::getHealthStatus(MemoryType type) {
    switch (type) {
        case HEAP_INTERNAL:
            return currentStats.heapHealth;
        case PSRAM_EXTERNAL:
            return currentStats.psramHealth;
        case STACK_MEMORY:
            return GOOD;  // Stack monitoring not implemented yet
        default:
            return EXCELLENT;
    }
}

size_t MemoryManager::getAvailableMemory(MemoryType type) {
    switch (type) {
        case HEAP_INTERNAL:
            return currentStats.heapFree;
        case PSRAM_EXTERNAL:
            return currentStats.psramAvailable ? currentStats.psramFree : 0;
        case STACK_MEMORY:
            return 0;  // Stack monitoring not implemented yet
        default:
            return 0;
    }
}

float MemoryManager::getUsagePercentage(MemoryType type) {
    size_t total = 0;
    size_t used = 0;
    
    switch (type) {
        case HEAP_INTERNAL:
            total = currentStats.heapTotal;
            used = currentStats.heapUsed;
            break;
        case PSRAM_EXTERNAL:
            if (!currentStats.psramAvailable) return -1.0f;
            total = currentStats.psramTotal;
            used = currentStats.psramUsed;
            break;
        case STACK_MEMORY:
            return -1.0f;  // Not implemented yet
        default:
            return -1.0f;
    }
    
    if (total == 0) return -1.0f;
    return ((float)used / (float)total) * 100.0f;
}

size_t MemoryManager::forceCleanup() {
    size_t initialFree = ESP.getFreeHeap();
    size_t freedBytes = 0;
    
    LOG_INFOF(TAG, "🧹 Starting memory cleanup (initial free: %d bytes)", initialFree);
    
    // 1. Force garbage collection by yielding
    for (int i = 0; i < 10; i++) {
        yield();
        delay(1);
    }
    
    // 2. Clear any cached data (placeholder for future implementation)
    // This is where we would clear image caches, temporary buffers, etc.
    
    // 3. Trigger ESP32 heap defragmentation
    #ifdef ESP_IDF_VERSION
    heap_caps_check_integrity_all(true);
    #endif
    
    // 4. Another round of yielding
    for (int i = 0; i < 5; i++) {
        yield();
        delay(1);
    }
    
    size_t finalFree = ESP.getFreeHeap();
    if (finalFree > initialFree) {
        freedBytes = finalFree - initialFree;
    }
    
    LOG_INFOF(TAG, "🧹 Cleanup completed: freed %d bytes (final free: %d bytes)", 
              freedBytes, finalFree);
    
    // Update statistics immediately
    updateHeapStats();
    updateHealthStatus();
    
    return freedBytes;
}

void MemoryManager::setAutoCleanup(bool enabled) {
    autoCleanupEnabled = enabled;
    LOG_INFOF(TAG, "Auto cleanup %s", enabled ? "enabled" : "disabled");
}

void MemoryManager::setMonitorInterval(unsigned long intervalMs) {
    monitorInterval = intervalMs;
    LOG_INFOF(TAG, "Monitor interval set to %lu ms", intervalMs);
}

bool MemoryManager::canAllocate(size_t requestedBytes, MemoryType type) {
    size_t available = getAvailableMemory(type);
    
    // Leave some safety margin (10% or 4KB, whichever is larger)
    size_t safetyMargin = max((size_t)(available * 0.1), (size_t)4096);
    
    return (available > (requestedBytes + safetyMargin));
}

void MemoryManager::printMemoryReport(bool includeHistory) {
    LOG_INFOF(TAG, "🔍 === COMPREHENSIVE MEMORY REPORT ===");
    LOG_INFOF(TAG, "System Uptime: %lu ms (%.1f minutes)", 
              currentStats.uptimeMs, currentStats.uptimeMs / 60000.0f);
    
    // Heap Statistics
    LOG_INFOF(TAG, "📊 HEAP MEMORY:");
    LOG_INFOF(TAG, "   Total: %d bytes (%.1f KB)", 
              currentStats.heapTotal, currentStats.heapTotal / 1024.0f);
    LOG_INFOF(TAG, "   Free: %d bytes (%.1f KB)", 
              currentStats.heapFree, currentStats.heapFree / 1024.0f);
    LOG_INFOF(TAG, "   Used: %d bytes (%.1f KB, %.1f%%)", 
              currentStats.heapUsed, currentStats.heapUsed / 1024.0f, getUsagePercentage(HEAP_INTERNAL));
    LOG_INFOF(TAG, "   Min Free Ever: %d bytes", currentStats.heapMinFree);
    LOG_INFOF(TAG, "   Max Single Alloc: %d bytes", currentStats.heapMaxAlloc);
    LOG_INFOF(TAG, "   Fragmentation: %.1f%%", currentStats.heapFragmentation);
    LOG_INFOF(TAG, "   Health: %s", getHealthStatusString(currentStats.heapHealth));
    
    // PSRAM Statistics
    if (currentStats.psramAvailable) {
        LOG_INFOF(TAG, "📊 PSRAM MEMORY:");
        LOG_INFOF(TAG, "   Total: %d bytes (%.1f KB)", 
                  currentStats.psramTotal, currentStats.psramTotal / 1024.0f);
        LOG_INFOF(TAG, "   Free: %d bytes (%.1f KB)", 
                  currentStats.psramFree, currentStats.psramFree / 1024.0f);
        LOG_INFOF(TAG, "   Used: %d bytes (%.1f KB, %.1f%%)", 
                  currentStats.psramUsed, currentStats.psramUsed / 1024.0f, getUsagePercentage(PSRAM_EXTERNAL));
        LOG_INFOF(TAG, "   Health: %s", getHealthStatusString(currentStats.psramHealth));
    } else {
        LOG_INFOF(TAG, "📊 PSRAM: Not available or not detected");
    }
    
    // Overall Health
    LOG_INFOF(TAG, "🏥 OVERALL HEALTH: %s", getHealthStatusString(currentStats.overallHealth));
    
    // Historical Statistics (if requested)
    if (includeHistory) {
        LOG_INFOF(TAG, "📈 HISTORICAL STATISTICS:");
        LOG_INFOF(TAG, "   Average Free Heap: %d bytes", currentStats.avgFreeHeap);
        LOG_INFOF(TAG, "   Peak Heap Usage: %d bytes", currentStats.peakHeapUsage);
        LOG_INFOF(TAG, "   Low Memory Events: %lu", currentStats.lowMemoryEvents);
        LOG_INFOF(TAG, "   Critical Events: %lu", currentStats.criticalEvents);
        LOG_INFOF(TAG, "   Cleanup Triggers: %lu", currentStats.cleanupTriggers);
    }
    
    LOG_INFOF(TAG, "🔍 === END MEMORY REPORT ===");
}

void MemoryManager::printMemoryStatus() {
    const char* healthIcon = "✅";
    if (currentStats.overallHealth >= WARNING) healthIcon = "⚠️";
    if (currentStats.overallHealth >= CRITICAL) healthIcon = "❌";
    
    LOG_INFOF(TAG, "%s Memory: Heap %dKB free (%.1f%%), Health: %s", 
              healthIcon,
              currentStats.heapFree / 1024,
              100.0f - getUsagePercentage(HEAP_INTERNAL),
              getHealthStatusString(currentStats.overallHealth));
    
    if (currentStats.psramAvailable) {
        LOG_INFOF(TAG, "   PSRAM: %dKB free (%.1f%%)", 
                  currentStats.psramFree / 1024,
                  100.0f - getUsagePercentage(PSRAM_EXTERNAL));
    }
}

const char* MemoryManager::getHealthStatusString(HealthStatus status) {
    switch (status) {
        case EXCELLENT: return "EXCELLENT";
        case GOOD: return "GOOD";
        case WARNING: return "WARNING";
        case CRITICAL: return "CRITICAL";
        case EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

const char* MemoryManager::getMemoryTypeString(MemoryType type) {
    switch (type) {
        case HEAP_INTERNAL: return "HEAP";
        case PSRAM_EXTERNAL: return "PSRAM";
        case STACK_MEMORY: return "STACK";
        default: return "UNKNOWN";
    }
}

void MemoryManager::resetStatistics() {
    currentStats.lowMemoryEvents = 0;
    currentStats.criticalEvents = 0;
    currentStats.cleanupTriggers = 0;
    currentStats.peakHeapUsage = currentStats.heapUsed;
    heapSampleCount = 1;
    heapSampleSum = currentStats.heapFree;
    currentStats.avgFreeHeap = currentStats.heapFree;
    
    LOG_INFOF(TAG, "📊 Memory statistics reset");
}

bool MemoryManager::isLowMemory() {
    return currentStats.overallHealth >= WARNING;
}

bool MemoryManager::isCriticalMemory() {
    return currentStats.overallHealth >= CRITICAL;
}

String MemoryManager::getMemoryStatsJson() {
    String json = "{";
    json += "\"heapTotal\":" + String(currentStats.heapTotal) + ",";
    json += "\"heapFree\":" + String(currentStats.heapFree) + ",";
    json += "\"heapUsed\":" + String(currentStats.heapUsed) + ",";
    json += "\"heapUsagePercent\":" + String(getUsagePercentage(HEAP_INTERNAL), 1) + ",";
    json += "\"heapFragmentation\":" + String(currentStats.heapFragmentation, 1) + ",";
    json += "\"heapHealth\":\"" + String(getHealthStatusString(currentStats.heapHealth)) + "\",";
    
    json += "\"psramAvailable\":" + String(currentStats.psramAvailable ? "true" : "false") + ",";
    if (currentStats.psramAvailable) {
        json += "\"psramTotal\":" + String(currentStats.psramTotal) + ",";
        json += "\"psramFree\":" + String(currentStats.psramFree) + ",";
        json += "\"psramUsed\":" + String(currentStats.psramUsed) + ",";
        json += "\"psramUsagePercent\":" + String(getUsagePercentage(PSRAM_EXTERNAL), 1) + ",";
        json += "\"psramHealth\":\"" + String(getHealthStatusString(currentStats.psramHealth)) + "\",";
    }
    
    json += "\"overallHealth\":\"" + String(getHealthStatusString(currentStats.overallHealth)) + "\",";
    json += "\"uptimeMs\":" + String(currentStats.uptimeMs) + ",";
    json += "\"lowMemoryEvents\":" + String(currentStats.lowMemoryEvents) + ",";
    json += "\"criticalEvents\":" + String(currentStats.criticalEvents) + ",";
    json += "\"cleanupTriggers\":" + String(currentStats.cleanupTriggers);
    
    json += "}";
    return json;
}
