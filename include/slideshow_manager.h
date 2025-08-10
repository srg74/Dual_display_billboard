#pragma once

#include <Arduino.h>
#include <vector>
#include <map>
#include <LittleFS.h>
#include "logger.h"
#include "image_manager.h"
#include "settings_manager.h"
#include "display_clock_manager.h"

class SlideshowManager {
private:
    ImageManager* imageManager;
    SettingsManager* settingsManager;
    DisplayClockManager* clockManager;
    
    // Slideshow state
    bool slideshowActive;
    unsigned long lastImageChange;
    unsigned long lastNoImagesCheck;  // NEW: Track timing for "no images" state
    int currentImageIndex;
    std::vector<String> enabledImages;
    std::map<String, bool> imageEnabledStates; // Track enabled/disabled state
    bool showingClock; // Track if currently showing clock
    
    // Internal methods
    void loadEnabledImages();
    bool isImageEnabled(const String& filename);
    void showNextImage();
    void showNoImagesMessage();
    
public:
    SlideshowManager(ImageManager* im, SettingsManager* sm, DisplayClockManager* cm);
    ~SlideshowManager();
    
    // Slideshow control
    void startSlideshow();
    void stopSlideshow();
    void updateSlideshow();
    void restartSlideshow(); // Reload images and restart
    
    // State queries
    bool isSlideshowActive() const { return slideshowActive; }
    bool shouldRetrySlideshow() const; // NEW: Check if enough time passed to retry when no images
    int getCurrentImageIndex() const { return currentImageIndex; }
    int getEnabledImageCount() const { return enabledImages.size(); }
    String getCurrentImageName() const;
    
    // Clock/gallery switching for DCC control
    void showClock();
    
    // Configuration
    void refreshImageList(); // Call when images are added/deleted/enabled/disabled
    void updateImageEnabledState(const String& filename, bool enabled);
    void loadImageStatesFromStorage();
    std::map<String, bool> getImageEnabledStates() const; // NEW: Get all enabled states
    
    // Initialization
    bool begin();
};
