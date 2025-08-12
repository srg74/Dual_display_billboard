/**
 * @file memory_manager.h
 * @brief Comprehensive memory monitoring and management system for ESP32-S3 dual display billboard
 * 
 * This module provides real-time memory monitoring, health tracking, and automatic
 * memory management for optimal system performance. Designed specifically for 
 * ESP32-S3 with PSRAM support and dual display configurations.
 * 
 * Key Features:
 * - Real-time heap and PSRAM monitoring
 * - Memory health scoring and alerts
 * - Automatic memory cleanup triggers
 * - Memory usage statistics and trending
 * - Integration with system logger
 * 
 * @author ESP32-S3 Billboard System
 * @date 2025
 * @version 1.0.0
 */

#pragma once
#include <Arduino.h>
#include "logger.h"
#include "config.h"

/**
 * @class MemoryManager
 * @brief Comprehensive memory monitoring and management system
 * 
 * Provides real-time monitoring of ESP32-S3 memory subsystems including
 * internal heap, PSRAM, and system health metrics. Includes automatic
 * cleanup triggers and memory optimization strategies.
 */
class MemoryManager {
public:
    /**
     * @brief Memory health status levels
     */
    enum HealthStatus {
        EXCELLENT = 0,  ///< >80% memory available, optimal performance
        GOOD = 1,       ///< >60% memory available, normal operation
        WARNING = 2,    ///< >40% memory available, monitor closely
        CRITICAL = 3,   ///< >20% memory available, cleanup needed
        EMERGENCY = 4   ///< <20% memory available, immediate action required
    };

    /**
     * @brief Memory subsystem types
     */
    enum MemoryType {
        HEAP_INTERNAL = 0,  ///< Internal ESP32 heap memory
        PSRAM_EXTERNAL = 1, ///< External PSRAM memory
        STACK_MEMORY = 2    ///< Stack memory usage
    };

    /**
     * @brief Memory statistics structure
     */
    struct MemoryStats {
        // Heap Statistics
        size_t heapTotal;         ///< Total heap size available
        size_t heapFree;          ///< Currently free heap memory
        size_t heapUsed;          ///< Currently used heap memory
        size_t heapMinFree;       ///< Minimum free heap ever recorded
        size_t heapMaxAlloc;      ///< Largest single allocation possible
        
        // PSRAM Statistics
        size_t psramTotal;        ///< Total PSRAM size available
        size_t psramFree;         ///< Currently free PSRAM memory
        size_t psramUsed;         ///< Currently used PSRAM memory
        bool psramAvailable;      ///< PSRAM detection status
        
        // Health Metrics
        HealthStatus overallHealth;  ///< Overall memory health status
        HealthStatus heapHealth;     ///< Heap-specific health status
        HealthStatus psramHealth;    ///< PSRAM-specific health status
        float heapFragmentation;     ///< Heap fragmentation percentage
        
        // Usage Statistics
        unsigned long uptimeMs;      ///< System uptime in milliseconds
        uint32_t lowMemoryEvents;    ///< Count of low memory events
        uint32_t criticalEvents;     ///< Count of critical memory events
        uint32_t cleanupTriggers;    ///< Count of automatic cleanups triggered
        
        // Performance Metrics
        size_t avgFreeHeap;          ///< Average free heap over time
        size_t peakHeapUsage;        ///< Peak heap usage recorded
        unsigned long lastUpdateMs;  ///< Last statistics update timestamp
    };

private:
    static const char* TAG;
    static MemoryStats currentStats;
    static unsigned long lastMonitorTime;
    static unsigned long monitorInterval;
    static bool autoCleanupEnabled;
    static size_t heapSampleCount;
    static size_t heapSampleSum;

    // Internal monitoring functions
    static void updateHeapStats();
    static void updatePsramStats();
    static void updateHealthStatus();
    static void updateFragmentation();
    static HealthStatus calculateHealthStatus(size_t free, size_t total);
    static void triggerCleanupIfNeeded();
    static void recordMemoryEvent(MemoryType type, HealthStatus status);

public:
    /**
     * @brief Initialize the memory monitoring system
     * @param monitorIntervalMs Monitoring update interval in milliseconds (default: 10000ms)
     * @param enableAutoCleanup Enable automatic memory cleanup (default: true)
     * @return true if initialization successful, false otherwise
     */
    static bool initialize(unsigned long monitorIntervalMs = 10000, bool enableAutoCleanup = true);

    /**
     * @brief Update memory statistics and perform health checks
     * 
     * Should be called regularly from main loop. Updates all memory statistics,
     * calculates health scores, and triggers cleanup if necessary.
     */
    static void update();

    /**
     * @brief Get current memory statistics
     * @return Reference to current memory statistics structure
     */
    static const MemoryStats& getStats();

    /**
     * @brief Get overall memory health status
     * @return Current memory health status
     */
    static HealthStatus getOverallHealth();

    /**
     * @brief Get memory health status for specific subsystem
     * @param type Memory subsystem type to check
     * @return Health status for specified memory type
     */
    static HealthStatus getHealthStatus(MemoryType type);

    /**
     * @brief Get available memory for specific subsystem
     * @param type Memory subsystem type to query
     * @return Available memory in bytes, 0 if not available
     */
    static size_t getAvailableMemory(MemoryType type);

    /**
     * @brief Calculate memory usage percentage
     * @param type Memory subsystem type to calculate
     * @return Usage percentage (0-100), or -1 if not available
     */
    static float getUsagePercentage(MemoryType type);

    /**
     * @brief Trigger immediate memory cleanup
     * 
     * Forces immediate cleanup of unnecessary memory allocations,
     * cache clearing, and garbage collection.
     * 
     * @return Number of bytes freed during cleanup
     */
    static size_t forceCleanup();

    /**
     * @brief Enable or disable automatic memory cleanup
     * @param enabled true to enable automatic cleanup, false to disable
     */
    static void setAutoCleanup(bool enabled);

    /**
     * @brief Set memory monitoring interval
     * @param intervalMs New monitoring interval in milliseconds
     */
    static void setMonitorInterval(unsigned long intervalMs);

    /**
     * @brief Check if memory is available for allocation
     * @param requestedBytes Number of bytes requested for allocation
     * @param type Memory type to check (default: HEAP_INTERNAL)
     * @return true if allocation is likely to succeed, false otherwise
     */
    static bool canAllocate(size_t requestedBytes, MemoryType type = HEAP_INTERNAL);

    /**
     * @brief Print comprehensive memory report to logger
     * @param includeHistory Include historical statistics in report (default: false)
     */
    static void printMemoryReport(bool includeHistory = false);

    /**
     * @brief Print compact memory status for regular monitoring
     */
    static void printMemoryStatus();

    /**
     * @brief Get human-readable health status string
     * @param status Health status to convert
     * @return Human-readable health status string
     */
    static const char* getHealthStatusString(HealthStatus status);

    /**
     * @brief Get human-readable memory type string
     * @param type Memory type to convert
     * @return Human-readable memory type string
     */
    static const char* getMemoryTypeString(MemoryType type);

    /**
     * @brief Reset memory statistics counters
     * 
     * Resets event counters and averages but preserves current readings.
     * Useful for long-running systems to prevent counter overflow.
     */
    static void resetStatistics();

    /**
     * @brief Check if system is in low memory condition
     * @return true if any memory subsystem is in WARNING or worse condition
     */
    static bool isLowMemory();

    /**
     * @brief Check if system is in critical memory condition
     * @return true if any memory subsystem is in CRITICAL or EMERGENCY condition
     */
    static bool isCriticalMemory();

    /**
     * @brief Get memory statistics as JSON string for web interface
     * @return JSON formatted memory statistics
     */
    static String getMemoryStatsJson();
};

// Convenience macros for easy memory monitoring
#define MEMORY_UPDATE() MemoryManager::update()
#define MEMORY_STATUS() MemoryManager::printMemoryStatus()
#define MEMORY_REPORT() MemoryManager::printMemoryReport()
#define MEMORY_HEALTH() MemoryManager::getOverallHealth()
#define MEMORY_IS_LOW() MemoryManager::isLowMemory()
#define MEMORY_IS_CRITICAL() MemoryManager::isCriticalMemory()
#define MEMORY_CAN_ALLOC(size) MemoryManager::canAllocate(size)
#define MEMORY_CLEANUP() MemoryManager::forceCleanup()
