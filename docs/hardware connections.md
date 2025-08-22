# Display Pin Configuration Reference

## Overview

This document provides the complete pin configuration for connecting ST7735 displays to ESP32 and ESP32-S3 boards in the dual display billboard system.

## Pin Assignments

### ESP32 (ESP32DEV_MODE)

Used for ESP32 development boards with standard pinout

| Function | Pin | GPIO | Description |
|----------|-----|------|-------------|
| **SPI Communication** | | | |
| MOSI | 23 | GPIO 23 | SPI Data (Master Out Slave In) |
| SCLK | 18 | GPIO 18 | SPI Clock |
| DC | 14 | GPIO 14 | Data/Command (shared for both displays) |
| RST | 26 | GPIO 26 | Reset (shared for both displays) |
| **Display 1** | | | |
| CS1 | 10 | GPIO 10 | Chip Select for Display 1 |
| BL1 | 7 | GPIO 7 | Backlight control for Display 1 |
| **Display 2** | | | |
| CS2 | 39 | GPIO 39 | Chip Select for Display 2 |
| BL2 | 8 | GPIO 8 | Backlight control for Display 2 |
| **DCC Input** | | | |
| DCC | 4 | GPIO 4 | Digital Command Control input signal |

### ESP32-S3 (ESP32S3_MODE)

Used for ESP32-S3-N8R2 modules with your specified pinout

| Function | Pin | GPIO | Description |
|----------|-----|------|-------------|
| **SPI Communication** | | | |
| MOSI | 11 | GPIO 11 | SPI Data (Master Out Slave In) |
| SCLK | 12 | GPIO 12 | SPI Clock |
| DC | 14 | GPIO 14 | Data/Command (shared for both displays) |
| RST | 5 | GPIO 5 | Reset (shared for both displays) |
| **Display 1** | | | |
| CS1 | 10 | GPIO 10 | Chip Select for Display 1 |
| BL1 | 7 | GPIO 7 | Backlight control for Display 1 |
| **Display 2** | | | |
| CS2 | 39 | GPIO 39 | Chip Select for Display 2 |
| BL2 | 8 | GPIO 8 | Backlight control for Display 2 |
| **DCC Input** | | | |
| DCC | 4 | GPIO 4 | Digital Command Control input signal |

## Simplified Connection Table - ESP32

```text
ESP32 Pin       →    Display 1    →    Display 2
──────────────────────────────────────────────────
3.3V            →    VCC          →    VCC (shared)
GND             →    GND          →    GND (shared)
GPIO 23 (MOSI)  →    SDA          →    SDA (shared)
GPIO 18 (SCLK)  →    SCL          →    SCL (shared)
GPIO 26 (RST)   →    RES          →    RES (shared)
GPIO 14 (DC)    →    DC           →    DC  (shared)
GPIO 10 (CS1)   →    CS           →    ─
GPIO 39 (CS2)   →    ─            →    CS
GPIO 7  (BL1)   →    BLK          →    ─
GPIO 8  (BL2)   →    ─            →    BLK
GPIO 4  (DCC)   →    DCC Input Signal
```

## Simplified Connection Table - ESP32-S3

```text
ESP32-S3 Pin    →    Display 1    →    Display 2
──────────────────────────────────────────────────
3.3V            →    VCC          →    VCC (shared)
GND             →    GND          →    GND (shared)
GPIO 11 (MOSI)  →    SDA          →    SDA (shared)
GPIO 12 (SCLK)  →    SCL          →    SCL (shared)
GPIO 5  (RST)   →    RES          →    RES (shared)
GPIO 14 (DC)    →    DC           →    DC  (shared)
GPIO 10 (CS1)   →    CS           →    ─
GPIO 39 (CS2)   →    ─                 →    CS
GPIO 7  (BL1)   →    BLK          →    ─
GPIO 8  (BL2)   →    ─            →    BLK
GPIO 4  (DCC)   →    DCC Input Signal
```

## Step-by-Step Wiring Guide - ESP32

### Power Connections - ESP32 (Do These First!)

```text
CRITICAL: Use 3.3V only - 5V will damage displays!

Step 1: Power Rails
ESP32 3.3V  →  Breadboard + rail  →  Both Display VCC pins
ESP32 GND   →  Breadboard - rail  →  Both Display GND pins
```

### SPI Bus Connections - ESP32 (Shared Between Displays)

```text
These pins connect to BOTH displays:

Step 2: SPI Data & Clock
ESP32 GPIO 23 (MOSI)  →  Display 1 SDA  →  Display 2 SDA
ESP32 GPIO 18 (SCLK)  →  Display 1 SCL  →  Display 2 SCL

Step 3: Control Signals  
ESP32 GPIO 26 (RST)   →  Display 1 RES  →  Display 2 RES
ESP32 GPIO 14 (DC)    →  Display 1 DC   →  Display 2 DC
```

### Individual Display Connections - ESP32

```text
These pins are unique for each display:

Step 4: Display 1 Only
ESP32 GPIO 10  →  Display 1 CS  pin
ESP32 GPIO 7   →  Display 1 BLK pin

Step 5: Display 2 Only  
ESP32 GPIO 39  →  Display 2 CS  pin
ESP32 GPIO 8   →  Display 2 BLK pin

Step 6: DCC Input Connection
ESP32 GPIO 4   →  DCC signal input
```

## Step-by-Step Wiring Guide - ESP32-S3

### Power Connections - ESP32-S3 (Do These First!)

```text
 CRITICAL: Use 3.3V only - 5V will damage displays!

Step 1: Power Rails
ESP32-S3 3.3V  →  Breadboard + rail  →  Both Display VCC pins
ESP32-S3 GND   →  Breadboard - rail  →  Both Display GND pins
```

### SPI Bus Connections - ESP32-S3 (Shared Between Displays)

```text
 These pins connect to BOTH displays:

Step 2: SPI Data & Clock
ESP32-S3 GPIO 11 (MOSI)  →  Display 1 SDA  →  Display 2 SDA
ESP32-S3 GPIO 12 (SCLK)  →  Display 1 SCL  →  Display 2 SCL

Step 3: Control Signals  
ESP32-S3 GPIO 5  (RST)   →  Display 1 RES  →  Display 2 RES
ESP32-S3 GPIO 14 (DC)    →  Display 1 DC   →  Display 2 DC
```

### Individual Display Connections - ESP32-S3

```text
These pins are unique for each display:

Step 4: Display 1 Only
ESP32-S3 GPIO 10  →  Display 1 CS  pin
ESP32-S3 GPIO 7   →  Display 1 BLK pin

Step 5: Display 2 Only  
ESP32-S3 GPIO 39  →  Display 2 CS  pin
ESP32-S3 GPIO 8   →  Display 2 BLK pin

Step 6: DCC Input Connection
ESP32-S3 GPIO 4   →  DCC signal input
```

## ST7735 Display Wiring

### Display Module Pins (typical ST7735 breakout)

- **VCC** → 3.3V (DO NOT use 5V)
- **GND** → Ground
- **SCL/SCLK** → SPI Clock pin (GPIO 12 for ESP32-S3, GPIO 18 for ESP32)
- **SDA/MOSI** → SPI Data pin (GPIO 11 for ESP32-S3, GPIO 23 for ESP32)
- **RES/RST** → GPIO 5 for ESP32-S3, GPIO 26 for ESP32 (shared between both displays)
- **DC/A0** → GPIO 14 (shared between both displays)
- **CS** → GPIO 10 (Display 1) or GPIO 39 (Display 2) for both platforms
- **BLK/LED** → GPIO 7 (Display 1) or GPIO 8 (Display 2) for both platforms

## Notes

### Power Requirements

- **Voltage**: 3.3V only - DO NOT connect to 5V
- **Current**: Each display draws ~20-50mA depending on content and backlight
- **Total system**: ~100-150mA for dual displays

### SPI Configuration

- **Mode**: SPI Mode 0 (CPOL=0, CPHA=0)
- **Speed**: 40MHz (optimized for ST7735)
- **Bit Order**: MSB First

### Display Specifications

- **Resolution**: 160x80 pixels (ST7735_GREENTAB160x80)
- **Color**: 16-bit RGB565
- **Orientation**: Portrait (80x160) or Landscape (160x80)

### Troubleshooting

1. **No display**: Check power (3.3V), connections, and CS pins
2. **Garbled display**: Verify SPI pins (MOSI, SCLK, DC)
3. **Wrong colors**: Check RGB order in configuration
4. **Flickering**: Check power supply stability and backlight control

## Hardware Testing Checklist

### Before Power-On

- [ ] Verify all connections match pin table above
- [ ] Confirm 3.3V power (NOT 5V)
- [ ] Check for short circuits
- [ ] Ensure CS pins are correctly connected (different for each display)

### After Power-On

- [ ] Check system boots correctly (serial monitor)
- [ ] Verify display initialization messages in log
- [ ] Test backlight control (displays should light up)
- [ ] Confirm both displays show content
- [ ] Test image upload and display functionality

### Common Issues

- **Power**: Insufficient power can cause boot loops or display corruption
- **DCC signal**: Ensure DCC signal is properly isolated and at correct voltage levels

## Configuration Validation

Current pin configuration is automatically selected based on the build environment:

- **esp32s3-st7735-debug**: Uses ESP32-S3 pinout
- **esp32dev-st7735-debug**: Uses ESP32 pinout

Check the serial monitor during boot for pin configuration confirmation.
