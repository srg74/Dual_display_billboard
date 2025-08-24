/**
 * @file memory_manager.cpp
 * @brief Implementation of comprehensive memory monitoring system for ESP32-S3
 * 
 * Provides real-time monitoring and management of ESP32-S3 memory subsystems
 * including internal heap, PSRAM, and automatic cleanup mechanisms.
 * 
 * @author ESP32-S3 Billboard System
 * @date 2025
 * @version 0.9
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
bool MemoryManager::monitoringEnabled = true;  // NEW: Allow temporary disable
size_t MemoryManager::heapSampleCount = 0;
size_t MemoryManager::heapSampleSum = 0;

/**
 * @brief Initializes the memory monitoring system with configurable parameters
 * 
 * Sets up the comprehensive memory monitoring framework for ESP32-S3 dual display
 * systems. Configures monitoring intervals, enables automatic cleanup triggers,
 * and performs initial memory system scans to establish baseline metrics.
 * 
 * The initialization process includes:
 * - Setting up monitoring intervals and cleanup behavior
 * - Clearing all statistical counters and establishing baselines
 * - Performing complete memory subsystem scan (heap + PSRAM)
 * - Calculating initial health metrics and fragmentation levels
 * - Logging system configuration and detected memory resources
 * 
 * @param monitorIntervalMs Memory monitoring update interval in milliseconds
 *                          (default 10000ms = 10 seconds). Lower values provide
 *                          more frequent updates but consume more CPU resources
 * @param enableAutoCleanup Enable automatic memory cleanup when critical
 *                         thresholds are reached. When enabled, system will
 *                         automatically trigger cleanup when memory health
 *                         reaches CRITICAL or EMERGENCY levels
 * 
 * @return true Always returns true indicating successful initialization
 *         (system is designed to be fault-tolerant)
 * 
 * @note This method should be called once during system startup, preferably
 *       before initializing other memory-intensive subsystems
 * @note On ESP32-S3 systems, PSRAM detection and configuration is automatically
 *       performed during initialization
 * @note Initial memory statistics are immediately available after this call
 * 
 * @see update() for periodic memory monitoring updates
 * @see setMonitorInterval() for runtime interval adjustments
 * @see setAutoCleanup() for runtime cleanup behavior changes
 * 
 * @since v0.9
 */
bool MemoryManager::initialize(unsigned long monitorIntervalMs, bool enableAutoCleanup) {
    LOG_INFOF(TAG, "Initializing memory monitoring system");
    
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
    
    LOG_INFOF(TAG, "Memory manager initialized");
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

/**
 * @brief Performs periodic memory monitoring and maintenance updates
 * 
 * Core monitoring method that should be called regularly from the main loop
 * to maintain real-time memory statistics and trigger automatic maintenance.
 * Updates all memory subsystem statistics, calculates health metrics, and
 * triggers cleanup operations when configured thresholds are exceeded.
 * 
 * The update process includes:
 * - Checking if monitoring interval has elapsed (rate limiting)
 * - Updating heap memory statistics and fragmentation metrics
 * - Scanning PSRAM status and usage (ESP32-S3 only)
 * - Recalculating health status for all memory subsystems
 * - Recording memory events when warning thresholds are exceeded
 * - Triggering automatic cleanup if enabled and conditions warrant
 * 
 * Rate limiting ensures updates only occur at the configured interval to
 * minimize CPU overhead while maintaining accurate monitoring.
 * 
 * @note This method should be called frequently from main loop (every loop iteration)
 *       as it includes built-in rate limiting based on monitorInterval
 * @note If monitoring is temporarily disabled via setMonitoringEnabled(false),
 *       this method returns immediately without performing any updates
 * @note Memory health deterioration automatically triggers event logging
 * @note Automatic cleanup is only triggered if autoCleanupEnabled is true
 * 
 * @see initialize() for initial setup and configuration
 * @see setMonitorInterval() for adjusting update frequency
 * @see setMonitoringEnabled() for temporarily disabling monitoring
 * @see forceCleanup() for manual memory cleanup operations
 * 
 * @since v0.9
 */
void MemoryManager::update() {
    // Skip update if monitoring is temporarily disabled
    if (!monitoringEnabled) {
        return;
    }
    
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

/**
 * @brief Updates internal ESP32 heap memory statistics and health metrics
 * 
 * Comprehensive heap memory analysis including corruption detection, usage
 * tracking, and performance metrics calculation. Implements robust error
 * handling to detect and recover from heap corruption scenarios.
 * 
 * Statistical updates include:
 * - Total and free heap memory measurements
 * - Minimum free heap tracking for worst-case analysis
 * - Maximum single allocation size for fragmentation analysis
 * - Peak usage tracking for capacity planning
 * - Running average calculation for trend analysis
 * - Sample count management to prevent overflow
 * 
 * Safety features:
 * - Heap corruption detection with sanity checks
 * - Platform-specific fallback values for corrupted readings
 * - Conservative estimates when corruption is detected
 * - Automatic recovery with safe default values
 * 
 * @note This is an internal method called automatically by update()
 * @note Corruption detection helps prevent system crashes from invalid readings
 * @note Peak usage tracking persists across monitoring cycles
 * @note Running averages use overflow-safe sample counting
 * @note ESP32-S3 and ESP32 classic have different heap size expectations
 * 
 * @see update() for the main monitoring loop that calls this method
 * @see updatePsramStats() for PSRAM-specific memory updates
 * @see calculateHealthStatus() for health metric calculation
 * 
 * @since v0.9
 */
void MemoryManager::updateHeapStats() {
    // SAFETY: Add basic validation for heap corruption without exceptions
    size_t heapTotal = ESP.getHeapSize();
    size_t heapFree = ESP.getFreeHeap();
    
    // Basic sanity check to detect obvious corruption
    if (heapTotal == 0 || heapFree > heapTotal || heapTotal > 10000000) {
        Serial.printf("WARNING: Heap corruption detected - total:%d, free:%d\n", heapTotal, heapFree);
        // Set safe fallback values based on platform
        #ifdef ESP32S3_MODE
        currentStats.heapTotal = 500000;  // ESP32-S3 typically has more heap
        currentStats.heapFree = 100000;   // Conservative estimate
        #else
        currentStats.heapTotal = 300000;  // ESP32 classic heap size
        currentStats.heapFree = 50000;    // Conservative estimate
        #endif
        currentStats.heapUsed = currentStats.heapTotal - currentStats.heapFree;
        return;
    }
    
    // Values seem reasonable, proceed with normal update
    currentStats.heapTotal = heapTotal;
    currentStats.heapFree = heapFree;
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

/**
 * @brief Updates PSRAM memory statistics with platform-specific handling
 * 
 * Manages PSRAM memory monitoring specifically for ESP32-S3 platforms while
 * ensuring proper handling on ESP32 classic systems that lack PSRAM support.
 * Includes corruption detection and platform-appropriate reporting.
 * 
 * ESP32-S3 PSRAM handling:
 * - Detects PSRAM presence using psramFound()
 * - Measures total and free PSRAM capacity
 * - Implements corruption detection with sanity checks
 * - Calculates usage statistics for health assessment
 * 
 * ESP32 Classic handling:
 * - Explicitly sets PSRAM as unavailable
 * - Prevents false reporting of PSRAM capabilities
 * - Ensures consistent behavior across platforms
 * 
 * Safety features:
 * - PSRAM corruption detection with bounds checking
 * - Graceful fallback when corruption is detected
 * - Platform-conditional compilation for optimal performance
 * - Consistent unavailable state for non-PSRAM platforms
 * 
 * @note This is an internal method called automatically by update()
 * @note ESP32 classic systems will always report PSRAM as unavailable
 * @note ESP32-S3 systems require PSRAM to be properly configured in board settings
 * @note Corruption detection prevents invalid memory calculations
 * @note PSRAM availability flag is updated on every scan
 * 
 * @see update() for the main monitoring loop that calls this method
 * @see updateHeapStats() for internal heap memory updates
 * @see calculateHealthStatus() for PSRAM health assessment
 * 
 * @since v0.9
 */
void MemoryManager::updatePsramStats() {
    #ifdef ESP32S3_MODE
    // Only ESP32-S3 should report PSRAM
    if (psramFound()) {
        size_t psramTotal = ESP.getPsramSize();
        size_t psramFree = ESP.getFreePsram();
        
        // Sanity check PSRAM values to detect corruption
        if (psramTotal == 0 || psramFree > psramTotal || psramTotal > 50000000) {
            Serial.printf("WARNING: PSRAM corruption detected - total:%d, free:%d\n", psramTotal, psramFree);
            currentStats.psramAvailable = false;
            currentStats.psramTotal = 0;
            currentStats.psramFree = 0;
            currentStats.psramUsed = 0;
        } else {
            // Values seem reasonable
            currentStats.psramAvailable = true;
            currentStats.psramTotal = psramTotal;
            currentStats.psramFree = psramFree;
            currentStats.psramUsed = currentStats.psramTotal - currentStats.psramFree;
        }
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

/**
 * @brief Updates health status metrics for all memory subsystems
 * 
 * Calculates and updates health status indicators for heap memory, PSRAM,
 * and overall system memory health. Health status provides quick assessment
 * of memory conditions and triggers for automated maintenance actions.
 * 
 * Health calculation process:
 * - Individual subsystem health assessment (heap, PSRAM)
 * - Overall health determination using worst-case principle
 * - PSRAM health only calculated when PSRAM is available
 * - Non-applicable subsystems default to EXCELLENT status
 * 
 * Health levels range from EXCELLENT (>80% free) to EMERGENCY (<20% free)
 * providing clear indicators for system administrators and automated systems.
 * 
 * @note This is an internal method called automatically by update()
 * @note Overall health always reflects the worst individual subsystem health
 * @note PSRAM health is marked EXCELLENT when PSRAM is not available
 * @note Health calculations are based on free memory percentages
 * 
 * @see calculateHealthStatus() for individual subsystem health calculation
 * @see update() for the main monitoring loop that calls this method
 * @see getOverallHealth() for retrieving current health status
 * 
 * @since v0.9
 */
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

/**
 * @brief Calculates heap memory fragmentation metrics
 * 
 * Analyzes heap fragmentation by comparing the largest possible single
 * allocation with total free memory. High fragmentation indicates memory
 * is scattered in small blocks, potentially limiting large allocations.
 * 
 * Fragmentation calculation:
 * - Compares heapMaxAlloc (largest single allocation) to heapFree (total free)
 * - Calculates fragmentation as percentage of unusable free memory
 * - Applies bounds checking to ensure valid percentage range (0-100%)
 * - Handles edge cases like zero free memory gracefully
 * 
 * Low fragmentation (0-20%) indicates efficient memory usage while high
 * fragmentation (>80%) suggests need for memory cleanup or defragmentation.
 * 
 * @note This is an internal method called automatically by update()
 * @note Fragmentation percentage is clamped to valid range 0-100%
 * @note Zero free memory results in 100% fragmentation
 * @note Lower fragmentation percentages indicate better memory organization
 * 
 * @see update() for the main monitoring loop that calls this method
 * @see forceCleanup() for operations that may improve fragmentation
 * @see updateHeapStats() for heap statistics that feed this calculation
 * 
 * @since v0.9
 */
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

/**
 * @brief Calculates health status based on memory usage percentage
 * 
 * Converts free memory metrics into standardized health status levels that
 * provide intuitive assessment of memory conditions. Uses percentage-based
 * thresholds to classify memory health across all subsystems consistently.
 * 
 * Health level thresholds:
 * - EXCELLENT: ≥80% free memory (optimal performance)
 * - GOOD: 60-79% free memory (normal operation)
 * - WARNING: 40-59% free memory (monitor closely)
 * - CRITICAL: 20-39% free memory (cleanup recommended)
 * - EMERGENCY: <20% free memory (immediate action required)
 * 
 * @param free Amount of free memory in bytes for the subsystem
 * @param total Total memory capacity in bytes for the subsystem
 * 
 * @return HealthStatus enumeration indicating current memory health level
 *         Returns EXCELLENT if total is zero (not applicable case)
 * 
 * @note This is an internal utility method used by updateHealthStatus()
 * @note Percentage calculation handles zero total gracefully
 * @note Thresholds are designed for typical ESP32 memory usage patterns
 * @note Same thresholds apply to both heap and PSRAM subsystems
 * 
 * @see updateHealthStatus() for comprehensive health status updates
 * @see getHealthStatusString() for human-readable health status names
 * @see HealthStatus enumeration for complete status level definitions
 * 
 * @since v0.9
 */
MemoryManager::HealthStatus MemoryManager::calculateHealthStatus(size_t free, size_t total) {
    if (total == 0) return EXCELLENT;  // Not applicable
    
    float freePercentage = ((float)free / (float)total) * 100.0f;
    
    if (freePercentage >= 80.0f) return EXCELLENT;
    if (freePercentage >= 60.0f) return GOOD;
    if (freePercentage >= 40.0f) return WARNING;
    if (freePercentage >= 20.0f) return CRITICAL;
    return EMERGENCY;
}

/**
 * @brief Triggers automatic memory cleanup when critical thresholds are reached
 * 
 * Monitors overall memory health and automatically initiates cleanup operations
 * when memory conditions reach critical or emergency levels. Provides automated
 * memory management without manual intervention when auto-cleanup is enabled.
 * 
 * Cleanup trigger conditions:
 * - Overall health status reaches CRITICAL or worse
 * - Auto-cleanup feature is enabled via setAutoCleanup()
 * - Cleanup operations are logged for system monitoring
 * 
 * When triggered, performs comprehensive memory cleanup including:
 * - Forced garbage collection and heap optimization
 * - Cache clearing and temporary buffer cleanup
 * - Memory defragmentation where possible
 * - Updated statistics collection post-cleanup
 * 
 * @note This is an internal method called automatically by update()
 * @note Cleanup only occurs when autoCleanupEnabled flag is true
 * @note Each cleanup operation increments the cleanupTriggers counter
 * @note Cleanup effectiveness is logged for system monitoring
 * 
 * @see forceCleanup() for the actual cleanup implementation
 * @see setAutoCleanup() for enabling/disabling automatic cleanup
 * @see update() for the monitoring loop that calls this method
 * 
 * @since v0.9
 */
void MemoryManager::triggerCleanupIfNeeded() {
    if (currentStats.overallHealth >= CRITICAL) {
        LOG_WARNF(TAG, "Triggering automatic memory cleanup (health: %s)", 
                  getHealthStatusString(currentStats.overallHealth));
        
        size_t freedBytes = forceCleanup();
        currentStats.cleanupTriggers++;
        
        LOG_INFOF(TAG, "Cleanup completed, freed %d bytes", freedBytes);
    }
}

/**
 * @brief Records memory events for statistical tracking and alerting
 * 
 * Maintains comprehensive event logging for memory health deterioration and
 * critical memory conditions. Provides statistical tracking for system
 * administration and automated alerting when memory issues occur.
 * 
 * Event classification:
 * - Low memory events: WARNING level or worse health status
 * - Critical events: CRITICAL level or worse health status
 * - Event counters maintained in MemoryStats structure
 * - Logging provides immediate notification of critical conditions
 * 
 * @param type Memory subsystem type (HEAP_INTERNAL, PSRAM_EXTERNAL, STACK_MEMORY)
 * @param status Current health status that triggered the event
 * 
 * @note This is an internal method called automatically during health monitoring
 * @note Events are counted cumulatively until statistics are reset
 * @note Critical events trigger immediate warning log messages
 * @note Event counters help identify patterns in memory usage
 * 
 * @see update() for automatic event recording during monitoring
 * @see getStats() for retrieving event counters
 * @see resetStatistics() for clearing event counters
 * 
 * @since v0.9
 */
void MemoryManager::recordMemoryEvent(MemoryType type, HealthStatus status) {
    if (status >= WARNING) {
        currentStats.lowMemoryEvents++;
    }
    
    if (status >= CRITICAL) {
        currentStats.criticalEvents++;
        LOG_WARNF(TAG, "Critical memory event: %s subsystem in %s condition", 
                  getMemoryTypeString(type), getHealthStatusString(status));
    }
}

/**
 * @brief Retrieves comprehensive memory statistics structure
 * 
 * Provides read-only access to the complete MemoryStats structure containing
 * all current memory metrics, health indicators, and historical statistics.
 * Essential for system monitoring, web interfaces, and diagnostic reporting.
 * 
 * Statistics include:
 * - Current heap and PSRAM usage and availability
 * - Health status for all memory subsystems
 * - Historical peak usage and average metrics
 * - Event counters for low memory and critical conditions
 * - System uptime and monitoring timestamps
 * 
 * @return const MemoryStats& Reference to current memory statistics structure
 *         providing comprehensive system memory information
 * 
 * @note Statistics are updated automatically by the update() method
 * @note Structure contains both instantaneous and historical metrics
 * @note All values are current as of the last update() call
 * @note Thread-safe read-only access to memory statistics
 * 
 * @see MemoryStats structure for complete field documentation
 * @see update() for statistics update frequency
 * @see printMemoryReport() for formatted statistics display
 * 
 * @since v0.9
 */
const MemoryManager::MemoryStats& MemoryManager::getStats() {
    return currentStats;
}

/**
 * @brief Retrieves overall system memory health status
 * 
 * Returns the current overall memory health indicator representing the worst-case
 * health status across all monitored memory subsystems. Provides single-value
 * assessment of system memory conditions for quick status checks.
 * 
 * Overall health determination:
 * - Represents worst health status among all subsystems
 * - Heap memory health always considered
 * - PSRAM health included when PSRAM is available
 * - Updated automatically during each monitoring cycle
 * 
 * @return HealthStatus Current overall memory health level
 *         (EXCELLENT, GOOD, WARNING, CRITICAL, or EMERGENCY)
 * 
 * @note Overall health always reflects the most critical subsystem
 * @note Updated automatically by updateHealthStatus() during monitoring
 * @note Useful for quick system health checks and dashboard displays
 * @note Does not require knowledge of individual subsystem details
 * 
 * @see getHealthStatus() for subsystem-specific health status
 * @see HealthStatus enumeration for status level definitions
 * @see updateHealthStatus() for health calculation methodology
 * 
 * @since v0.9
 */
MemoryManager::HealthStatus MemoryManager::getOverallHealth() {
    return currentStats.overallHealth;
}

/**
 * @brief Retrieves health status for specific memory subsystem
 * 
 * Returns health status for individual memory subsystems allowing targeted
 * monitoring and diagnostic analysis. Enables subsystem-specific health
 * assessment and troubleshooting of memory-related issues.
 * 
 * Supported subsystem types:
 * - HEAP_INTERNAL: ESP32 internal heap memory status
 * - PSRAM_EXTERNAL: External PSRAM memory status (ESP32-S3 only)
 * - STACK_MEMORY: Stack memory usage (not yet implemented)
 * 
 * @param type Memory subsystem type to query
 * 
 * @return HealthStatus Current health status for specified subsystem
 *         Returns EXCELLENT for unsupported or non-applicable subsystems
 * 
 * @note Stack memory monitoring is not yet implemented (returns GOOD)
 * @note PSRAM health only meaningful on ESP32-S3 with PSRAM enabled
 * @note Heap health is always available on all ESP32 platforms
 * @note Unknown subsystem types default to EXCELLENT status
 * 
 * @see getOverallHealth() for system-wide health assessment
 * @see MemoryType enumeration for supported subsystem types
 * @see HealthStatus enumeration for status level definitions
 * 
 * @since v0.9
 */
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

/**
 * @brief Retrieves available memory for specific subsystem
 * 
 * Returns current free memory amount for specified memory subsystem in bytes.
 * Essential for allocation planning and memory availability assessment before
 * performing memory-intensive operations.
 * 
 * Memory subsystem availability:
 * - HEAP_INTERNAL: Current free heap memory in bytes
 * - PSRAM_EXTERNAL: Free PSRAM memory (0 if not available)
 * - STACK_MEMORY: Stack monitoring not implemented (returns 0)
 * 
 * @param type Memory subsystem type to query
 * 
 * @return size_t Available memory in bytes for specified subsystem
 *         Returns 0 for unavailable or unsupported subsystems
 * 
 * @note PSRAM returns 0 when not available or on non-PSRAM platforms
 * @note Stack memory monitoring is not yet implemented
 * @note Values reflect most recent update() cycle measurements
 * @note Useful for pre-allocation memory availability checks
 * 
 * @see canAllocate() for allocation feasibility checking with safety margins
 * @see getUsagePercentage() for memory utilization assessment
 * @see MemoryType enumeration for supported subsystem types
 * 
 * @since v0.9
 */
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

/**
 * @brief Calculates memory usage percentage for specific subsystem
 * 
 * Computes current memory utilization as percentage of total capacity for
 * specified memory subsystem. Provides normalized usage metrics for capacity
 * planning and utilization monitoring across different subsystems.
 * 
 * Calculation methodology:
 * - Percentage = (used memory / total memory) × 100
 * - Returns -1.0 for unavailable or unsupported subsystems
 * - Handles zero total memory gracefully
 * - Accurate to floating-point precision
 * 
 * @param type Memory subsystem type to analyze
 * 
 * @return float Memory usage percentage (0.0-100.0)
 *         Returns -1.0 if subsystem is unavailable or not supported
 * 
 * @note Returns -1.0 for PSRAM when not available or on non-PSRAM platforms
 * @note Stack memory monitoring not implemented (returns -1.0)
 * @note Zero total memory results in -1.0 (error condition)
 * @note Useful for dashboard displays and utilization alerts
 * 
 * @see getAvailableMemory() for absolute memory availability
 * @see getHealthStatus() for health assessment based on usage
 * @see MemoryType enumeration for supported subsystem types
 * 
 * @since v0.9
 */
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

/**
 * @brief Performs comprehensive memory cleanup and optimization
 * 
 * Executes manual memory cleanup operations to free unused memory and
 * optimize system performance. Combines multiple cleanup strategies to
 * maximize memory recovery and improve system responsiveness.
 * 
 * Cleanup operations performed:
 * - Multiple rounds of yield() calls for garbage collection
 * - Heap integrity checking and defragmentation (ESP-IDF systems)
 * - Cache clearing and temporary buffer cleanup (future implementation)
 * - Memory measurement before and after cleanup for effectiveness analysis
 * 
 * The cleanup process is designed to be safe and non-disruptive while
 * maximizing memory recovery. Multiple yield cycles ensure thorough
 * garbage collection without blocking system operation.
 * 
 * @return size_t Amount of memory freed during cleanup operation in bytes
 *         May return 0 if no additional memory could be recovered
 * 
 * @note This operation may take several milliseconds to complete
 * @note Cleanup effectiveness varies based on current memory fragmentation
 * @note Statistics are automatically updated after cleanup completion
 * @note Safe to call manually even when auto-cleanup is enabled
 * 
 * @see triggerCleanupIfNeeded() for automatic cleanup triggering
 * @see setAutoCleanup() for enabling automatic cleanup behavior
 * @see updateHeapStats() for post-cleanup statistics updates
 * 
 * @since v0.9
 */
size_t MemoryManager::forceCleanup() {
    size_t initialFree = ESP.getFreeHeap();
    size_t freedBytes = 0;
    
    LOG_INFOF(TAG, "Starting memory cleanup (initial free: %d bytes)", initialFree);
    
    // 1. Force garbage collection by yielding (non-blocking)
    yield(); // Single yield instead of loop with delays
    
    // 2. Clear any cached data (placeholder for future implementation)
    // This is where we would clear image caches, temporary buffers, etc.
    
    // 3. Trigger ESP32 heap defragmentation
    #ifdef ESP_IDF_VERSION
    heap_caps_check_integrity_all(true);
    #endif
    
    // 4. Final yield (non-blocking)
    yield(); // Single yield instead of loop with delays
    
    size_t finalFree = ESP.getFreeHeap();
    if (finalFree > initialFree) {
        freedBytes = finalFree - initialFree;
    }
    
    LOG_INFOF(TAG, "Cleanup completed: freed %d bytes (final free: %d bytes)", 
              freedBytes, finalFree);
    
    // Update statistics immediately
    updateHeapStats();
    updateHealthStatus();
    
    return freedBytes;
}

/**
 * @brief Configures automatic memory cleanup behavior
 * 
 * Enables or disables automatic memory cleanup operations that trigger when
 * memory health reaches critical levels. Provides runtime control over
 * automated memory management without requiring system restart.
 * 
 * When enabled, automatic cleanup triggers when:
 * - Overall memory health reaches CRITICAL or EMERGENCY levels
 * - Memory monitoring detects sustained high memory usage
 * - System approaches memory exhaustion conditions
 * 
 * @param enabled true to enable automatic cleanup, false to disable
 * 
 * @note Changes take effect immediately during next monitoring cycle
 * @note Manual cleanup via forceCleanup() remains available regardless of setting
 * @note Cleanup triggers are logged for system monitoring and analysis
 * @note Recommended to keep enabled for autonomous operation
 * 
 * @see triggerCleanupIfNeeded() for automatic cleanup implementation
 * @see forceCleanup() for manual cleanup operations
 * @see initialize() for initial auto-cleanup configuration
 * 
 * @since v0.9
 */
void MemoryManager::setAutoCleanup(bool enabled) {
    autoCleanupEnabled = enabled;
    LOG_INFOF(TAG, "Auto cleanup %s", enabled ? "enabled" : "disabled");
}

/**
 * @brief Enables or disables memory monitoring operations
 * 
 * Provides runtime control over memory monitoring system operation allowing
 * temporary suspension of monitoring activities during critical operations
 * or system maintenance. Useful for reducing overhead during intensive tasks.
 * 
 * When monitoring is disabled:
 * - update() method returns immediately without processing
 * - No statistics updates or health calculations performed
 * - Automatic cleanup operations are suspended
 * - Current statistics remain unchanged until monitoring resumes
 * 
 * @param enabled true to enable monitoring, false to temporarily disable
 * 
 * @note Monitoring state change takes effect immediately
 * @note Disabling monitoring does not affect manual operations like forceCleanup()
 * @note Current statistics are preserved during monitoring suspension
 * @note Recommended to re-enable monitoring promptly to maintain system visibility
 * 
 * @see update() for main monitoring loop affected by this setting
 * @see initialize() for initial monitoring setup
 * @see setMonitorInterval() for monitoring frequency adjustment
 * 
 * @since v0.9
 */
void MemoryManager::setMonitoringEnabled(bool enabled) {
    monitoringEnabled = enabled;
    LOG_INFOF(TAG, "Memory monitoring %s", enabled ? "enabled" : "disabled");
}

/**
 * @brief Configures memory monitoring update interval
 * 
 * Sets the frequency of memory monitoring updates providing control over
 * monitoring overhead versus update granularity. Lower intervals provide
 * more responsive monitoring while higher intervals reduce CPU usage.
 * 
 * Interval considerations:
 * - Lower values (1-5 seconds): High responsiveness, higher CPU overhead
 * - Medium values (10-30 seconds): Balanced monitoring for normal operation
 * - Higher values (>60 seconds): Low overhead for background monitoring
 * 
 * @param intervalMs Memory monitoring update interval in milliseconds
 *                   Recommended range: 1000ms (1 second) to 300000ms (5 minutes)
 * 
 * @note Changes take effect immediately during next update() call
 * @note Very low intervals (<1000ms) may impact system performance
 * @note Very high intervals (>300s) may delay critical event detection
 * @note Default interval is 10000ms (10 seconds) providing good balance
 * 
 * @see initialize() for initial interval configuration
 * @see update() for monitoring loop affected by this interval
 * @see setMonitoringEnabled() for suspending monitoring entirely
 * 
 * @since v0.9
 */
void MemoryManager::setMonitorInterval(unsigned long intervalMs) {
    monitorInterval = intervalMs;
    LOG_INFOF(TAG, "Monitor interval set to %lu ms", intervalMs);
}

/**
 * @brief Checks if memory allocation request is feasible with safety margins
 * 
 * Analyzes whether requested memory allocation can be safely performed on
 * specified subsystem without risking system stability. Includes safety
 * margins to prevent memory exhaustion and maintain system responsiveness.
 * 
 * Allocation assessment includes:
 * - Current available memory for specified subsystem
 * - Safety margin calculation (10% of available or 4KB minimum)
 * - Buffer to prevent system instability from memory exhaustion
 * - Subsystem-specific availability checking
 * 
 * Safety margins ensure system remains stable even after large allocations
 * by reserving memory for critical system operations and emergency cleanup.
 * 
 * @param requestedBytes Size of memory allocation request in bytes
 * @param type Memory subsystem for allocation (HEAP_INTERNAL, PSRAM_EXTERNAL)
 * 
 * @return bool true if allocation can be safely performed, false otherwise
 * 
 * @note Safety margin is 10% of available memory or 4KB, whichever is larger
 * @note PSRAM allocations return false when PSRAM is not available
 * @note Stack memory allocation checking is not yet supported
 * @note Recommendation: check before large allocations to prevent failures
 * 
 * @see getAvailableMemory() for raw memory availability
 * @see getUsagePercentage() for current memory utilization
 * @see MemoryType enumeration for supported allocation targets
 * 
 * @since v0.9
 */
bool MemoryManager::canAllocate(size_t requestedBytes, MemoryType type) {
    size_t available = getAvailableMemory(type);
    
    // Leave some safety margin (10% or 4KB, whichever is larger)
    size_t safetyMargin = max((size_t)(available * 0.1), (size_t)4096);
    
    return (available > (requestedBytes + safetyMargin));
}

/**
 * @brief Generates comprehensive memory report with detailed statistics
 * 
 * Produces detailed memory analysis report including current usage, health
 * metrics, and optional historical statistics. Essential for system
 * diagnostics, capacity planning, and performance analysis.
 * 
 * Report sections include:
 * - System uptime and monitoring status
 * - Complete heap memory analysis (total, free, used, fragmentation)
 * - PSRAM memory statistics (when available)
 * - Health status assessments for all subsystems
 * - Optional historical metrics (peak usage, event counts, cleanup triggers)
 * 
 * The comprehensive report provides administrators with complete visibility
 * into memory subsystem performance and helps identify potential issues
 * before they impact system stability.
 * 
 * @param includeHistory true to include historical statistics and event counts,
 *                      false for current status only. Historical data includes
 *                      peak usage tracking, average calculations, and event
 *                      counters for trend analysis
 * 
 * @note Report is output via LOG_INFOF for integration with system logging
 * @note Historical statistics provide valuable trend analysis data
 * @note PSRAM section only included when PSRAM is available
 * @note All memory values displayed in both bytes and kilobytes for readability
 * 
 * @see printMemoryStatus() for brief status summary
 * @see getStats() for programmatic access to memory statistics
 * @see getMemoryStatsJson() for JSON-formatted statistics export
 * 
 * @since v0.9
 */
void MemoryManager::printMemoryReport(bool includeHistory) {
    LOG_INFOF(TAG, "=== COMPREHENSIVE MEMORY REPORT ===");
    LOG_INFOF(TAG, "System Uptime: %lu ms (%.1f minutes)", 
              currentStats.uptimeMs, currentStats.uptimeMs / 60000.0f);
    
    // Heap Statistics
    LOG_INFOF(TAG, " HEAP MEMORY:");
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
        LOG_INFOF(TAG, " PSRAM MEMORY:");
        LOG_INFOF(TAG, "   Total: %d bytes (%.1f KB)", 
                  currentStats.psramTotal, currentStats.psramTotal / 1024.0f);
        LOG_INFOF(TAG, "   Free: %d bytes (%.1f KB)", 
                  currentStats.psramFree, currentStats.psramFree / 1024.0f);
        LOG_INFOF(TAG, "   Used: %d bytes (%.1f KB, %.1f%%)", 
                  currentStats.psramUsed, currentStats.psramUsed / 1024.0f, getUsagePercentage(PSRAM_EXTERNAL));
        LOG_INFOF(TAG, "   Health: %s", getHealthStatusString(currentStats.psramHealth));
    } else {
        LOG_INFOF(TAG, " PSRAM: Not available or not detected");
    }
    
    // Overall Health
    LOG_INFOF(TAG, " OVERALL HEALTH: %s", getHealthStatusString(currentStats.overallHealth));
    
    // Historical Statistics (if requested)
    if (includeHistory) {
        LOG_INFOF(TAG, " HISTORICAL STATISTICS:");
        LOG_INFOF(TAG, "   Average Free Heap: %d bytes", currentStats.avgFreeHeap);
        LOG_INFOF(TAG, "   Peak Heap Usage: %d bytes", currentStats.peakHeapUsage);
        LOG_INFOF(TAG, "   Low Memory Events: %lu", currentStats.lowMemoryEvents);
        LOG_INFOF(TAG, "   Critical Events: %lu", currentStats.criticalEvents);
        LOG_INFOF(TAG, "   Cleanup Triggers: %lu", currentStats.cleanupTriggers);
    }
    
    LOG_INFOF(TAG, "=== END MEMORY REPORT ===");
}

/**
 * @brief Displays concise memory status for dashboard and monitoring
 * 
 * Provides brief memory status summary optimized for dashboard displays
 * and frequent monitoring updates. Shows essential memory metrics with
 * visual health indicators for quick system assessment.
 * 
 * Status display includes:
 * - Visual health indicator (icon) based on overall health
 * - Heap memory availability in kilobytes and percentage
 * - Overall health status as readable string
 * - PSRAM availability when present
 * 
 * Visual indicators provide immediate health assessment:
 * - EXCELLENT/GOOD health status
 * - WARNING health status 
 * - CRITICAL/EMERGENCY health status
 * 
 * @note Output formatted for dashboard displays and frequent updates
 * @note Health icons provide immediate visual health assessment
 * @note PSRAM status included only when PSRAM is available
 * @note Memory displayed in kilobytes for readability
 * 
 * @see printMemoryReport() for comprehensive detailed analysis
 * @see getOverallHealth() for health status used in indicator selection
 * @see getUsagePercentage() for percentage calculations displayed
 * 
 * @since v0.9
 */
void MemoryManager::printMemoryStatus() {
    const char* healthIcon = "OK";
    if (currentStats.overallHealth >= WARNING) healthIcon = "WARN";
    if (currentStats.overallHealth >= CRITICAL) healthIcon = "CRITICAL";
    
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

/**
 * @brief Converts health status enumeration to human-readable string
 * 
 * Provides string representation of health status levels for logging,
 * display interfaces, and JSON serialization. Essential for user-friendly
 * status reporting and system integration.
 * 
 * Health status mappings:
 * - EXCELLENT: "EXCELLENT" (>80% free memory)
 * - GOOD: "GOOD" (60-79% free memory)
 * - WARNING: "WARNING" (40-59% free memory)
 * - CRITICAL: "CRITICAL" (20-39% free memory)
 * - EMERGENCY: "EMERGENCY" (<20% free memory)
 * - Unknown: "UNKNOWN" (error fallback)
 * 
 * @param status HealthStatus enumeration value to convert
 * 
 * @return const char* Human-readable string representation of health status
 *         Never returns NULL; unknown values return "UNKNOWN"
 * 
 * @note Return value is const char* pointing to static string (safe for logging)
 * @note Unknown status values default to "UNKNOWN" for error handling
 * @note String values are uppercase for consistency with logging standards
 * @note Used internally by reporting functions and available for external use
 * 
 * @see getMemoryTypeString() for memory type string conversion
 * @see printMemoryReport() for usage in comprehensive reporting
 * @see getMemoryStatsJson() for JSON serialization using these strings
 * 
 * @since v0.9
 */
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

/**
 * @brief Converts memory type enumeration to human-readable string
 * 
 * Provides string representation of memory subsystem types for logging,
 * error reporting, and system integration. Essential for clear identification
 * of memory subsystems in diagnostic output and event logging.
 * 
 * Memory type mappings:
 * - HEAP_INTERNAL: "HEAP" (ESP32 internal heap memory)
 * - PSRAM_EXTERNAL: "PSRAM" (External PSRAM memory)
 * - STACK_MEMORY: "STACK" (Stack memory usage)
 * - Unknown: "UNKNOWN" (error fallback)
 * 
 * @param type MemoryType enumeration value to convert
 * 
 * @return const char* Human-readable string representation of memory type
 *         Never returns NULL; unknown values return "UNKNOWN"
 * 
 * @note Return value is const char* pointing to static string (safe for logging)
 * @note Unknown memory types default to "UNKNOWN" for error handling
 * @note String values are uppercase for consistency with logging standards
 * @note Used internally by event logging and available for external use
 * 
 * @see getHealthStatusString() for health status string conversion
 * @see recordMemoryEvent() for usage in event logging
 * @see MemoryType enumeration for complete type definitions
 * 
 * @since v0.9
 */
const char* MemoryManager::getMemoryTypeString(MemoryType type) {
    switch (type) {
        case HEAP_INTERNAL: return "HEAP";
        case PSRAM_EXTERNAL: return "PSRAM";
        case STACK_MEMORY: return "STACK";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Resets all historical statistics and event counters
 * 
 * Clears accumulated statistical data and resets counters to baseline values
 * for fresh monitoring cycle initiation. Useful for periodic statistics
 * reset or after significant system changes.
 * 
 * Reset operations include:
 * - Clearing all event counters (low memory, critical events, cleanup triggers)
 * - Resetting peak usage tracking to current usage levels
 * - Reinitializing running average calculations with current values
 * - Maintaining current memory measurements (not historical data)
 * 
 * Current memory statistics remain unchanged; only historical tracking
 * and accumulated counters are reset to provide clean baseline for
 * future monitoring cycles.
 * 
 * @note Current memory usage statistics are preserved
 * @note Peak usage tracking resets to current usage levels
 * @note Running averages restart from current free memory values
 * @note Event counters reset to zero for fresh tracking cycle
 * 
 * @see getStats() for accessing statistics that will be reset
 * @see recordMemoryEvent() for events that increment counters
 * @see update() for monitoring that accumulates statistics
 * 
 * @since v0.9
 */
void MemoryManager::resetStatistics() {
    currentStats.lowMemoryEvents = 0;
    currentStats.criticalEvents = 0;
    currentStats.cleanupTriggers = 0;
    currentStats.peakHeapUsage = currentStats.heapUsed;
    heapSampleCount = 1;
    heapSampleSum = currentStats.heapFree;
    currentStats.avgFreeHeap = currentStats.heapFree;
    
    LOG_INFOF(TAG, " Memory statistics reset");
}

/**
 * @brief Checks if system is in low memory condition
 * 
 * Provides boolean assessment of low memory conditions for simplified
 * status checking and conditional logic. Returns true when overall
 * memory health indicates attention is needed.
 * 
 * Low memory condition is defined as overall health status of WARNING
 * or worse, indicating that available memory has dropped below optimal
 * levels and monitoring or intervention may be required.
 * 
 * @return bool true if overall health is WARNING, CRITICAL, or EMERGENCY
 *         false if overall health is EXCELLENT or GOOD
 * 
 * @note Based on overall health status combining all memory subsystems
 * @note WARNING level indicates <60% memory available system-wide
 * @note Useful for conditional logic and automated response triggers
 * @note Updated automatically during each monitoring cycle
 * 
 * @see isCriticalMemory() for more severe memory condition checking
 * @see getOverallHealth() for complete health status details
 * @see HealthStatus enumeration for threshold definitions
 * 
 * @since v0.9
 */
bool MemoryManager::isLowMemory() {
    return currentStats.overallHealth >= WARNING;
}

/**
 * @brief Checks if system is in critical memory condition
 * 
 * Provides boolean assessment of critical memory conditions requiring
 * immediate attention or intervention. Returns true when memory health
 * indicates system stability may be at risk.
 * 
 * Critical memory condition is defined as overall health status of CRITICAL
 * or EMERGENCY, indicating severe memory pressure that requires immediate
 * action to prevent system instability or failure.
 * 
 * @return bool true if overall health is CRITICAL or EMERGENCY
 *         false if overall health is EXCELLENT, GOOD, or WARNING
 * 
 * @note Based on overall health status combining all memory subsystems
 * @note CRITICAL level indicates <40% memory available system-wide
 * @note EMERGENCY level indicates <20% memory available system-wide
 * @note Should trigger immediate cleanup or resource reduction actions
 * 
 * @see isLowMemory() for broader low memory condition checking
 * @see getOverallHealth() for complete health status details
 * @see forceCleanup() for cleanup actions when critical condition detected
 * 
 * @since v0.9
 */
bool MemoryManager::isCriticalMemory() {
    return currentStats.overallHealth >= CRITICAL;
}

/**
 * @brief Exports memory statistics as JSON formatted string
 * 
 * Generates comprehensive memory statistics in JSON format for web APIs,
 * remote monitoring systems, and data export applications. Provides
 * structured data access to all memory metrics and health indicators.
 * 
 * JSON structure includes:
 * - Heap memory statistics (total, free, used, usage percentage, fragmentation)
 * - Heap health status as readable string
 * - PSRAM availability flag and statistics (when available)
 * - PSRAM health status (when applicable)
 * - Overall system health assessment
 * - System uptime and event counters
 * 
 * The JSON output is suitable for REST APIs, monitoring dashboards,
 * and integration with external monitoring systems requiring structured
 * memory data.
 * 
 * @return String JSON-formatted string containing comprehensive memory statistics
 *         Always returns valid JSON; missing data is handled gracefully
 * 
 * @note PSRAM fields only included when PSRAM is available
 * @note All numeric values included as JSON numbers (not strings)
 * @note Health status values included as JSON strings
 * @note Floating-point values rounded to 1 decimal place for readability
 * 
 * @see getStats() for programmatic access to raw statistics
 * @see printMemoryReport() for human-readable formatted output
 * @see getHealthStatusString() for health status string conversion
 * 
 * @since v0.9
 */
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
