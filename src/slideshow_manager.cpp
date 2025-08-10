#include "slideshow_manager.h"

SlideshowManager::SlideshowManager(ImageManager* im, SettingsManager* sm, DisplayClockManager* cm) 
    : imageManager(im), settingsManager(sm), clockManager(cm) {
    slideshowActive = false;
    lastImageChange = 0;
    lastNoImagesCheck = 0;  // Initialize the new timing variable
    currentImageIndex = 0;
    showingClock = false;
}

SlideshowManager::~SlideshowManager() {
    // Nothing to clean up
}

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

void SlideshowManager::startSlideshow() {
    Serial.println("=== Starting Slideshow ===");
    
    loadEnabledImages();
    
    if (enabledImages.empty()) {
        Serial.println("No enabled images found - showing 'No Images' message");
        slideshowActive = false;
        showNoImagesMessage();
        lastNoImagesCheck = millis(); // Set timing to prevent immediate restart
        return;
    }
    
    Serial.printf("Found %d enabled images for slideshow\n", enabledImages.size());
    slideshowActive = true;
    currentImageIndex = 0;
    lastImageChange = millis();
    lastNoImagesCheck = 0; // Reset no images check when we have images
    
    // Display first image immediately
    showNextImage();
}

void SlideshowManager::stopSlideshow() {
    Serial.println("=== Stopping Slideshow ===");
    slideshowActive = false;
}

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

void SlideshowManager::restartSlideshow() {
    Serial.println("=== Restarting Slideshow ===");
    if (slideshowActive) {
        stopSlideshow();
        startSlideshow();
    }
}

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

String SlideshowManager::getCurrentImageName() const {
    if (slideshowActive && currentImageIndex < enabledImages.size()) {
        return enabledImages[currentImageIndex];
    }
    return "";
}

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
    uint32_t checkInterval = 10000; // 10 seconds as requested
    if (settingsManager) {
        checkInterval = settingsManager->getImageInterval() * 1000; // Convert to ms
        // Minimum 10 seconds to avoid rapid loops
        if (checkInterval < 10000) {
            checkInterval = 10000;
        }
    }
    
    return (currentTime - lastNoImagesCheck >= checkInterval);
}

void SlideshowManager::showNextImage() {
    if (enabledImages.empty() || !imageManager) {
        return;
    }
    
    String currentImage = enabledImages[currentImageIndex];
    Serial.printf("Slideshow: Showing image %d/%d: %s\n", 
                 currentImageIndex + 1, enabledImages.size(), currentImage.c_str());
    
    // Display on both screens or based on display settings
    bool result = imageManager->displayImageOnBoth(currentImage);
    Serial.printf("Image display result: %s\n", result ? "SUCCESS" : "FAILED");
}

void SlideshowManager::loadEnabledImages() {
    enabledImages.clear();
    
    Serial.println("Loading enabled images...");
    
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

bool SlideshowManager::isImageEnabled(const String& filename) {
    // Check if we have a stored state for this image
    auto it = imageEnabledStates.find(filename);
    if (it != imageEnabledStates.end()) {
        return it->second;
    }
    
    // Default to enabled if no state is stored
    return true;
}

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
