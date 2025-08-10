#pragma once

#include <Arduino.h>
#include <vector>
#include <map>
#include <LittleFS.h>
#include "logger.h"
#include "image_manager.h"
#include "settings_manager.h"

class SlideshowManager {
private:
    ImageManager* imageManager;
    SettingsManager* settingsManager;
    
    // Slideshow state
    bool slideshowActive;
    unsigned long lastImageChange;
    int currentImageIndex;
    std::vector<String> enabledImages;
    std::map<String, bool> imageEnabledStates; // Track enabled/disabled state
    
    // Internal methods
    void loadEnabledImages();
    bool isImageEnabled(const String& filename);
    void showNextImage();
    void showNoImagesMessage();
    
public:
    SlideshowManager(ImageManager* im, SettingsManager* sm);
    ~SlideshowManager();
    
    // Slideshow control
    void startSlideshow();
    void stopSlideshow();
    void updateSlideshow();
    void restartSlideshow(); // Reload images and restart
    
    // State queries
    bool isSlideshowActive() const { return slideshowActive; }
    int getCurrentImageIndex() const { return currentImageIndex; }
    int getEnabledImageCount() const { return enabledImages.size(); }
    String getCurrentImageName() const;
    
    // Configuration
    void refreshImageList(); // Call when images are added/deleted/enabled/disabled
    void updateImageEnabledState(const String& filename, bool enabled);
    void loadImageStatesFromStorage();
    
    // Initialization
    bool begin();
};
