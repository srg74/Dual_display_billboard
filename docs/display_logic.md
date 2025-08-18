# Display Logic Reference
**Dual Display Billboard System - Hardware Constraints & Logic Documentation**

## Hardware Constraints (FIXED - No Pin Changes Possible)
- **ESP32DEV Mode**: CS1=GPIO5, CS2=GPIO15, DC=GPIO14, BLK1=GPIO22, BLK2=GPIO27
- **ESP32S3 Mode**: CS1=GPIO10, CS2=GPIO39, DC=GPIO14, BLK1=GPIO7, BLK2=GPIO8
- **Shared SPI Bus**: All displays share MOSI, SCLK, RST pins
- **Independent Control**: Each display controlled via CS (Chip Select) pins
- **No delay() Usage**: Professional project uses millis() timing and yield()

## Display Numbering Convention
- **Display 1**: Primary display (CS1 pin control)
- **Display 2**: Secondary display (CS2 pin control)  
- **Display 0**: Both displays simultaneously

## Screen Rotation Logic
- **Text Rotation**: `TEXT_ROTATION = 3` (landscape orientation)
- **Image Rotation**: `IMAGE_ROTATION = 0` (portrait orientation)
- **Automatic Selection**: 
  - `selectDisplayForText()` - Uses TEXT_ROTATION
  - `selectDisplayForImage()` - Uses IMAGE_ROTATION

## Displays default logic and settings:

Case when firmware uploaded to a empty controller flash memory:

1. Splash screen for 4000ms (4 seconds) to show user controller is alive.

- by default display1 is primary for showing information
- display2 remain dark (whatever settings might be applicable) until user will enable display 2 in normal mode. Only in case of restart of controller saved display2 state was different from default.

2. Controller start in AP portal mode.

- portal info appears only on display1 (green background, with black font), display2 remain dark. 

- after user input credential to a network in web page and click connect or hit enter on keyboard display show "Connecting..." on display1 only for period of connection.

3. Upon successful connection to a network:

- Blue screen with connection info (already in code) stay on display1 for 5 seconds and controller goes to normal mode.

4. If network connection is not established:

- Red screen with message: "Network connection failed!" (Message in 2 lines) on display1 only

Case when you have fully functional controller and power outage or something else cause reboot:

Controller read configuration from saved files and start as configured.

## Display State Machine & Timing

### 1. System Startup Sequence
```
Startup → Display Init → WiFi Init → Time Init → Ready
   ↓           ↓           ↓          ↓        ↓
 Boot      Hardware    Connection   NTP     Normal
Timer     Validation   Attempt     Sync   Operation
```

### 2. Portal/Setup Mode Logic
```
AP Mode Detected → Splash Screen (5s) → Portal Info → Hold
                    Both Displays     Display 1      Until Exit
```
**Timing**: 
- Splash: 5000ms (5 seconds) on both displays
- Portal Info: Indefinite hold on Display 1, Display 2 dark

### 3. Normal Operation Mode Logic

#### When Images Enabled & Available:
```
Image 1 → Image 2 → ... → Image N → [Clock] → Loop
  ↓         ↓               ↓         ↓
Display  Display         Display   Display
Interval Interval       Interval  Interval
```
**Timing**:
- Image Interval: User configurable (settingsManager.getImageInterval() * 1000ms)
- Clock Display: Only if clock enabled in settings
- Both displays show same content

#### When No Images or Images Disabled:
```
Display 1: BLUE → Display 2: YELLOW → Display 1: BLUE → ...
    ↓                   ↓                   ↓
  3000ms              3000ms              3000ms
```
**Timing**: alternateDisplays() switches every 3000ms (3 seconds)

### 4. Status Display Priority System
```
Priority 1: Splash Screen (blocks all other displays)
Priority 2: Portal/Setup Info (blocks normal operation)
Priority 3: Connection Success Message (temporary)
Priority 4: Normal Operation (Images/Clock/Alternate)
```

## Current Display Logic Implementation

### Core Display Manager Functions:
- `showSplashScreen(displayNum, timeoutMs)` - Non-blocking splash with timeout
- `showPortalSequence(ssid, ip, status)` - 5s splash → portal info
- `showPortalInfo(ssid, ip, status)` - Display 1: green background, Display 2: dark
- `alternateDisplays()` - Display 1: blue, Display 2: yellow, 3s intervals
- `updateSplashScreen()` - Non-blocking timeout handler (call in main loop)

### Slideshow Manager Logic:
- `startSlideshow()` - Loads images, starts sequence
- `updateSlideshow()` - Non-blocking progression with user timing
- Clock insertion: After last image IF clock enabled
- Image progression: Circular (wraps to first after last)

### WiFi Integration:
- Setup Mode: Portal sequence active, normal operation blocked
- Normal Mode: Full slideshow/clock operation
- Connection Success: Temporary override of normal display

## Non-Blocking Timing Architecture

All timing uses `millis()` comparison patterns:
```cpp
static unsigned long lastAction = 0;
unsigned long currentTime = millis();

if (currentTime - lastAction >= INTERVAL) {
    // Perform action
    lastAction = currentTime;
}
```

**Critical**: No `delay()` calls anywhere in professional implementation.

## Display Content Distribution

### Current Assignment Logic:
- **Both Displays Same Content**: Images, Clock, Splash
- **Display 1 Only**: Portal info, Status messages  
- **Display 2 Dark**: During portal/status displays
- **Alternating**: Blue/Yellow when no content available

### Memory Management:
- Single TFT_eSPI instance shared between displays
- Display selection via CS pin control
- Rotation set per display type (text vs image)
- No sprite buffering in standard mode
- PSRAM utilization for ESP32-S3 advanced graphics

## Future Refactoring Considerations

1. **Centralized State Machine**: Consolidate all display logic decisions
2. **Abstract Display Interface**: Hide TFT_eSPI implementation details  
3. **Configurable Timing**: Move hardcoded timings to configuration
4. **Event-Driven Architecture**: Replace polling with event triggers
5. **Hardware Abstraction Layer**: Support different display types cleanly

---
**Note**: This document reflects current working implementation. 
Any changes must maintain backward compatibility and require team approval + testing.
