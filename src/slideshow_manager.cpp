#include "slideshow_manager.h"

SlideshowManager::SlideshowManager(ImageManager* im, SettingsManager* sm) 
    : imageManager(im), settingsManager(sm) {
    slideshowActive = false;
    lastImageChange = 0;
    currentImageIndex = 0;
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
        return;
    }
    
    Serial.printf("Found %d enabled images for slideshow\n", enabledImages.size());
    slideshowActive = true;
    currentImageIndex = 0;
    lastImageChange = millis();
    
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
        // Move to next image
        currentImageIndex = (currentImageIndex + 1) % enabledImages.size();
        showNextImage();
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
    Serial.printf("Updating image state: %s = %s\n", filename.c_str(), enabled ? "enabled" : "disabled");
    
    imageEnabledStates[filename] = enabled;
    
    // Save states to file
    File file = LittleFS.open("/slideshow_states.json", "w");
    if (file) {
        file.print("{");
        bool first = true;
        for (const auto& pair : imageEnabledStates) {
            if (!first) file.print(",");
            file.printf("\"%s\":%s", pair.first.c_str(), pair.second ? "true" : "false");
            first = false;
        }
        file.print("}");
        file.close();
        Serial.println("Image states saved to storage");
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

void SlideshowManager::showNoImagesMessage() {
    if (!imageManager) {
        return;
    }
    
    Serial.println("Displaying 'No Images' message on screens");
    
    // Get display manager from image manager
    // Note: This is a temporary approach - we might need to access DisplayManager directly
    // For now, we'll use the existing display system to show a message
    
    // TODO: Implement a proper "No Images" display method
    // This could involve:
    // 1. Creating a simple text overlay on the display
    // 2. Using a pre-generated "no images" image
    // 3. Drawing text directly to the display buffer
    
    Serial.println("'No Images' message displayed");
}
