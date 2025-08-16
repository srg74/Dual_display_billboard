/**
 * @file slideshow_manager.cpp
 * @brief 🎯 Advanced slideshow management system with clock integration and DCC control
 * 
 * This module provides comprehensive slideshow functionality for dual display billboards,
 * featuring intelligent image sequencing, clock integration, DCC command response,
 * and dynamic image management with real-time enable/disable capabilities.
 * 
 * Key Features:
 * - Configurable image timing and transition control
 * - Seamless clock integration between image sequences
 * - Dynamic image list management with enable/disable states
 * - DCC command integration for remote gallery/clock switching
 * - Persistent image state storage and recovery
 * - Smart retry logic for "no images" scenarios
 * 
 * @author ESP32 Dual Display Billboard System
 * @date 2025
 * @version 0.9
 */

#include "slideshow_manager.h"
#include "display_timing_config.h"

// ============================================================================
// CONSTRUCTOR AND INITIALIZATION
// ============================================================================

/**
/**
 * @brief 🏗️ Constructor initializes slideshow manager with required component dependencies
 * @param im Pointer to ImageManager for image handling operations
 * @param sm Pointer to SettingsManager for configuration access
 * @param cm Pointer to DisplayClockManager for clock display integration
 * 
 * Initializes the slideshow manager with all required component dependencies
 * and sets default state values for slideshow operation.
 */
SlideshowManager::SlideshowManager(ImageManager* im, SettingsManager* sm, DisplayClockManager* cm) 
    : imageManager(im), settingsManager(sm), clockManager(cm) {
    slideshowActive = false;
    lastImageChange = 0;
    lastNoImagesCheck = 0;  // Initialize the new timing variable
    currentImageIndex = 0;
    showingClock = false;
}

/**
 * @brief 🔚 Destructor performs cleanup operations
 * 
 * Currently no dynamic resources require cleanup, but provides
 * framework for future resource management if needed.
 */
SlideshowManager::~SlideshowManager() {
    // Nothing to clean up
}

/**
 * @brief 🚀 Initialize slideshow manager and load persistent image states
 * @return true if initialization successful, false if required components unavailable
 * 
 * Performs initial setup including validation of required component dependencies
 * and loading of persistent image enable/disable states from storage.
 */
bool SlideshowManager::begin() {
    if (!imageManager || !settingsManager) {
        LOG_ERROR("SLIDESHOW", "ImageManager or SettingsManager not available");
        return false;
    }
    
    // Load image enabled states from storage
    loadImageStatesFromStorage();
    
    LOG_INFO("SLIDESHOW", "SlideshowManager initialized");
    return true;
}

// ============================================================================
// SLIDESHOW CONTROL AND MANAGEMENT
// ============================================================================

/**
 * @brief ▶️ Start slideshow with automatic image loading and validation
 * 
 * Initiates slideshow operation by loading enabled images and starting
 * the display sequence. Handles "no images" scenario gracefully with
 * appropriate messaging and timing controls.
 */
void SlideshowManager::startSlideshow() {
    loadEnabledImages();
    
    if (enabledImages.empty()) {
        slideshowActive = false;
        showNoImagesMessage();
        lastNoImagesCheck = millis(); // Set timing to prevent immediate restart
        return;
    }
    
    slideshowActive = true;
    currentImageIndex = 0;
    lastImageChange = millis();
    lastNoImagesCheck = 0; // Reset no images check when we have images
    
    // Display first image immediately
    showNextImage();
}

/**
 * @brief ⏹️ Stop slideshow and reset state variables
 * 
 * Halts slideshow operation and resets internal state variables
 * to prepare for future slideshow sessions.
 */
void SlideshowManager::stopSlideshow() {
    slideshowActive = false;
    currentImageIndex = 0;
}

/**
 * @brief 🔄 Update slideshow timing and handle image/clock transitions
 * 
 * Core slideshow update loop that handles timing-based image transitions,
 * clock integration based on user settings, and maintains proper sequence
 * flow between images and clock displays.
 */
void SlideshowManager::updateSlideshow() {
    if (!slideshowActive || enabledImages.empty() || !settingsManager) {
        return;
    }
    
    unsigned long currentTime = millis();
    uint32_t imageInterval = settingsManager->getImageInterval() * 1000; // Convert seconds to ms
    
    if (currentTime - lastImageChange >= imageInterval) {
        // Check if clock is enabled and we've shown all images
        bool clockEnabled = settingsManager->isClockEnabled();
        
        if (clockEnabled && !showingClock && currentImageIndex == enabledImages.size() - 1) {
            // Time to show clock after last image
            showingClock = true;
            showClock();
        } else if (showingClock) {
            // Clock was showing, now return to first image
            showingClock = false;
            currentImageIndex = 0;
            showNextImage();
        } else {
            // Normal image progression
            currentImageIndex = (currentImageIndex + 1) % enabledImages.size();
            showNextImage();
        }
        
        lastImageChange = currentTime;
    }
}

/**
 * @brief 🔄 Restart active slideshow with fresh image loading
 * 
 * Performs complete slideshow restart including stopping current operation
 * and reinitializing with fresh image list. Only operates when slideshow
 * is currently active to prevent unwanted state changes.
 */
void SlideshowManager::restartSlideshow() {
    Serial.println("=== Restarting Slideshow ===");
    if (slideshowActive) {
        stopSlideshow();
        startSlideshow();
    }
}

/**
 * @brief 🔄 Refresh image list with dynamic slideshow state management
 * 
 * Reloads the image list and intelligently manages slideshow state based
 * on image availability. Handles transitions between active/inactive states
 * and adjusts current index when image list changes.
 */

void SlideshowManager::refreshImageList() {
    Serial.println("=== Refreshing Image List ===");
    
    // Store previous state
    bool wasActive = slideshowActive;
    size_t previousCount = enabledImages.size();
    
    loadEnabledImages();
    
    Serial.printf("Image count changed: %d -> %d\n", previousCount, enabledImages.size());
    
    if (enabledImages.empty()) {
        // No enabled images - show "no images" message
        if (wasActive) {
            Serial.println("No enabled images - stopping slideshow and showing message");
            slideshowActive = false;
            showNoImagesMessage();
        }
    } else {
        // We have enabled images
        if (!wasActive) {
            // Slideshow was stopped, restart it
            Serial.println("Images now available - starting slideshow");
            startSlideshow();
        } else {
            // Slideshow was active, adjust index if needed
            if (currentImageIndex >= enabledImages.size()) {
                currentImageIndex = 0;
                Serial.println("Adjusting image index due to list change");
            }
            
            // Continue with current or adjusted image
            showNextImage();
        }
    }
}

// ============================================================================
// STATE QUERIES AND STATUS METHODS
// ============================================================================

/**
 * @brief 📋 Get name of currently displayed image
 * @return Current image filename if slideshow active, empty string otherwise
 * 
 * Returns the filename of the currently displayed image when slideshow
 * is active and valid. Provides safe access with bounds checking.
 */
String SlideshowManager::getCurrentImageName() const {
    if (slideshowActive && currentImageIndex < enabledImages.size()) {
        return enabledImages[currentImageIndex];
    }
    return "";
}

/**
 * @brief ⏰ Check if slideshow should retry after "no images" state
 * @return true if enough time has passed since last retry attempt
 * 
 * Implements intelligent retry logic to prevent rapid slideshow restart
 * attempts when no images are available. Uses configurable timing intervals
 * to balance responsiveness with system stability.
 */

bool SlideshowManager::shouldRetrySlideshow() const {
    // If slideshow is active, no need to retry
    if (slideshowActive) {
        return false;
    }
    
    // If we've never checked for no images, allow retry
    if (lastNoImagesCheck == 0) {
        return true;
    }
    
    // Check if enough time has passed since last "no images" check
    // Use the same interval as image display (default 30 seconds, configurable to 10)
    unsigned long currentTime = millis();
    uint32_t checkInterval = SLIDESHOW_DEFAULT_INTERVAL_MS; // 10 seconds as requested
    if (settingsManager) {
        checkInterval = settingsManager->getImageInterval() * 1000; // Convert to ms
        // Minimum 10 seconds to avoid rapid loops
        if (checkInterval < SLIDESHOW_DEFAULT_INTERVAL_MS) {
            checkInterval = SLIDESHOW_DEFAULT_INTERVAL_MS;
        }
    }
    
    return (currentTime - lastNoImagesCheck >= checkInterval);
}

// ============================================================================
// INTERNAL HELPER METHODS
// ============================================================================

/**
/**
 * @brief 🖼️ Display next image in sequence on available displays
 * 
 * Handles the actual image display operation using ImageManager to show
 * the current image on both displays. Includes validation and error handling
 * for missing images or manager components.
 */
void SlideshowManager::showNextImage() {
    if (enabledImages.empty() || !imageManager) {
        return;
    }
    
    String currentImage = enabledImages[currentImageIndex];
    
    // Display on both screens or based on display settings
    bool result = imageManager->displayImageOnBoth(currentImage);
}

/**
 * @brief 📂 Load list of enabled images from filesystem and user preferences
 * 
 * Scans the images directory and builds a list of enabled images based on
 * user preferences and file availability. Filters for supported image formats
 * and respects enable/disable state settings.
 */
void SlideshowManager::loadEnabledImages() {
    enabledImages.clear();
    
    // Get list of all images from the images directory
    File dir = LittleFS.open("/images");
    if (!dir || !dir.isDirectory()) {
        Serial.println("Failed to open images directory");
        return;
    }
    
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = file.name();
            if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
                if (isImageEnabled(filename)) {
                    enabledImages.push_back(filename);
                    Serial.printf("Added enabled image: %s\n", filename.c_str());
                }
            }
        }
        file = dir.openNextFile();
    }
    dir.close();
    
    Serial.printf("Total enabled images: %d\n", enabledImages.size());
}

/**
 * @brief ✅ Check if specific image is enabled for slideshow display
 * @param filename Image filename to check enable status
 * @return true if image is enabled, false if disabled, defaults to true if not found
 * 
 * Queries the image enabled states map to determine if a specific image
 * should be included in slideshow rotation. Uses default enabled state
 * for images without explicit settings.
 */
bool SlideshowManager::isImageEnabled(const String& filename) {
    // Check if we have a stored state for this image
    auto it = imageEnabledStates.find(filename);
    if (it != imageEnabledStates.end()) {
        return it->second;
    }
    
    // Default to enabled if no state is stored
    return true;
}

// ============================================================================
// CONFIGURATION AND STATE MANAGEMENT
// ============================================================================

/**
 * @brief 🔧 Update enable/disable state for specific image in slideshow
 * @param filename Image filename to update state for
 * @param enabled New enabled state (true=enabled, false=disabled)
 * 
 * Updates the enabled state for a specific image and persists the change
 * to storage. Automatically refreshes the slideshow to apply changes
 * immediately if the slideshow is currently active.
 */

void SlideshowManager::updateImageEnabledState(const String& filename, bool enabled) {
    Serial.printf("=== UPDATING IMAGE STATE ===\n");
    Serial.printf("Updating image state: %s = %s\n", filename.c_str(), enabled ? "enabled" : "disabled");
    
    imageEnabledStates[filename] = enabled;
    Serial.printf("Map now contains %d entries\n", imageEnabledStates.size());
    
    // Save states to file
    File file = LittleFS.open("/slideshow_states.json", "w");
    if (file) {
        String json = "{";
        bool first = true;
        for (const auto& pair : imageEnabledStates) {
            if (!first) json += ",";
            json += "\"" + pair.first + "\":" + (pair.second ? "true" : "false");
            first = false;
        }
        json += "}";
        
        file.print(json);
        file.close();
        Serial.printf("Saved JSON to file: %s\n", json.c_str());
        Serial.println("Image states saved to storage successfully");
    } else {
        Serial.println("ERROR: Failed to open /slideshow_states.json for writing");
    }
    
    // Refresh slideshow if needed
    refreshImageList();
}

/**
 * @brief 💾 Load image enable/disable states from persistent storage
 * 
 * Reads image enabled states from JSON file stored in LittleFS filesystem.
 * Handles missing file gracefully by defaulting all images to enabled state.
 * Performs simple JSON parsing to restore user preferences for image visibility.
 */

void SlideshowManager::loadImageStatesFromStorage() {
    Serial.println("Loading image enabled states from storage...");
    
    File file = LittleFS.open("/slideshow_states.json", "r");
    if (!file) {
        Serial.println("No stored image states found - all images will be enabled by default");
        return;
    }
    
    String content = file.readString();
    file.close();
    
    // Simple JSON parsing for states
    // Format: {"image1.jpg":true,"image2.jpg":false}
    content.trim();
    if (content.startsWith("{") && content.endsWith("}")) {
        content = content.substring(1, content.length() - 1); // Remove { }
        
        int pos = 0;
        while (pos < content.length()) {
            int colonPos = content.indexOf(':', pos);
            int commaPos = content.indexOf(',', pos);
            if (commaPos == -1) commaPos = content.length();
            
            if (colonPos > pos && colonPos < commaPos) {
                String filename = content.substring(pos, colonPos);
                String enabled = content.substring(colonPos + 1, commaPos);
                
                // Clean quotes
                filename.replace("\"", "");
                enabled.trim();
                
                imageEnabledStates[filename] = (enabled == "true");
                Serial.printf("Loaded state: %s = %s\n", filename.c_str(), enabled.c_str());
            }
            
            pos = commaPos + 1;
        }
    }
    
    Serial.printf("Loaded %d image states from storage\n", imageEnabledStates.size());
}

/**
 * @brief 📋 Get comprehensive map of all image enabled states for web interface
 * @return Map of image filenames to their enabled states (true/false)
 * 
 * Returns complete map of image enabled states, including both stored preferences
 * and default enabled states for newly discovered images. Essential for web
 * interface to display current image enable/disable status to users.
 */

std::map<String, bool> SlideshowManager::getImageEnabledStates() const {
    Serial.println("=== GET IMAGE ENABLED STATES ===");
    std::map<String, bool> result = imageEnabledStates;
    Serial.printf("Starting with %d stored states\n", result.size());
    
    // Ensure all existing images are included in the result, defaulting to enabled
    File dir = LittleFS.open("/images");
    if (dir && dir.isDirectory()) {
        File file = dir.openNextFile();
        int newDefaults = 0;
        while (file) {
            if (!file.isDirectory()) {
                String filename = file.name();
                if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
                    // If this image doesn't have a stored state, default to enabled
                    if (result.find(filename) == result.end()) {
                        result[filename] = true;
                        newDefaults++;
                        Serial.printf("Added default enabled state for: %s\n", filename.c_str());
                    }
                }
            }
            file = dir.openNextFile();
        }
        dir.close();
        Serial.printf("Added %d default enabled states\n", newDefaults);
    } else {
        Serial.println("ERROR: Could not open /images directory");
    }
    
    Serial.printf("Returning %d total states:\n", result.size());
    for (const auto& pair : result) {
        Serial.printf("  %s = %s\n", pair.first.c_str(), pair.second ? "enabled" : "disabled");
    }
    
    return result;
}

/**
 * @brief 📷 Display "No Images Available" message on both displays
 * 
 * Shows user-friendly message when no enabled images are available for slideshow.
 * Utilizes ImageManager's display functionality to render text message on both
 * screens, providing clear feedback about slideshow state.
 */

void SlideshowManager::showNoImagesMessage() {
    if (!imageManager) {
        return;
    }
    
    Serial.println("Displaying 'No Images' message on screens");
    
    // Use the image manager's display functionality to show "No Images" message
    // This will use the display manager internally to show text on both screens
    imageManager->showNoImagesMessage();
    
    Serial.println("'No Images' message displayed");
}

/**
 * @brief 🕐 Display clock on both screens with user-configured face style
 * 
 * Renders clock display using DisplayClockManager with user's preferred clock
 * face style from settings. Integrates with slideshow sequence to provide
 * time display between image rotations when clock feature is enabled.
 */

void SlideshowManager::showClock() {
    if (!clockManager) {
        Serial.println("ClockManager not available");
        return;
    }
    
    Serial.println("Slideshow: Showing clock");
    
    // Set the clock face from settings before displaying
    if (settingsManager) {
        ClockFaceType currentFace = settingsManager->getClockFace();
        clockManager->setClockFace(currentFace);
    }
    
    // Display clock on both screens
    clockManager->displayClockOnBothDisplays();
    
    Serial.println("Clock display complete");
}
