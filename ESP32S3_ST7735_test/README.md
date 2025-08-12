# ESP32-S3 ST7735 Display Test

A minimal test project to verify ST7735 display functionality with ESP32-S3 using the working configuration from the main project.

## Hardware Requirements

- ESP32-S3 development board (tested with ESP32-S3-DevKitC-1)
- ST7735 0.96" 80x160 TFT display (GREENTAB variant)
- Jumper wires for connections

## Pin Connections

| ST7735 Pin | ESP32-S3 GPIO | Function |
|------------|---------------|----------|
| VCC        | 3.3V          | Power    |
| GND        | GND           | Ground   |
| CS         | GPIO 10       | Chip Select |
| DC         | GPIO 14       | Data/Command |
| RST        | GPIO 4        | Reset    |
| SDA (MOSI) | GPIO 11       | SPI Data |
| SCL (SCLK) | GPIO 12       | SPI Clock |
| MISO       | GPIO 13       | SPI Input (optional) |
| BLK (BL)   | GPIO 7        | Backlight |

## Features

This test program demonstrates:

1. **Display Initialization**: Proper ST7735 setup with ESP32-S3 using TFT_eSPI
2. **Color Tests**: Fill screen with different colors  
3. **Text Rendering**: Display text in various colors
4. **Shape Drawing**: Rectangles, circles, lines, and pixels (optimized for 80x160)
5. **Runtime Display**: Shows elapsed time and free heap memory
6. **PSRAM Support**: Uses QSPI PSRAM mode (working configuration)

## Building and Uploading

1. Open this project in VS Code with PlatformIO extension
2. Connect your ESP32-S3 board via USB
3. Build and upload:

   ```bash
   pio run -t upload
   ```

4. Monitor serial output:

   ```bash
   pio device monitor
   ```

## Expected Output

- Serial monitor will show initialization progress and test status
- Display will show color fills, text, and geometric shapes
- Backlight will have a breathing effect
- Runtime counter updates every 5 seconds

## Troubleshooting

If the display doesn't work:

1. **Check Connections**: Verify all pins are connected correctly
2. **Power Supply**: Ensure 3.3V is stable and sufficient  
3. **SPI Configuration**: Verify MOSI/SCLK pins match your ESP32-S3 board
4. **Display Variant**: This code is configured for GREENTAB variant. If you have a different ST7735, try:
   - Change `-DST7735_GREENTAB160x80` to `-DST7735_BLACKTAB` or `-DST7735_REDTAB`
   - Adjust width/height if needed
5. **PSRAM Issues**: If boot problems occur, the QSPI PSRAM configuration should work with most ESP32-S3 modules

## Library Dependencies

- TFT_eSPI by Bodmer

This library provides better performance and ESP32-S3 compatibility compared to Adafruit libraries.
