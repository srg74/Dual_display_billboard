# Hardware Folder Overview

This folder contains all hardware-related resources for the ST7735 Dual Display Billboard project.

## Contents

### Schematic Documentation

- **ESP32 dual ST7735 display controller.pdf**  
  Complete circuit schematic for the ESP32 dual display controller.

- **ESP32S3 dual ST7735 display controller.pdf**  
  Complete circuit schematic for the ESP32-S3 dual display controller.

### Manufacturing Files

- **ESP32/**  
  Production files for ESP32 variant:
  - `BOM-ESP32_dual_ST7735_display.csv`: Bill of Materials
  - `CPL-ESP32_dual_ST7735_display.csv`: Component Placement List
  - `GERBER-ESP32_dual_ST7735_display.zip`: Gerber files for PCB fabrication

- **ESP32S3/**  
  Production files for ESP32-S3 variant:
  - `BOM-ESP32S3_dual_ST7735_display.csv`: Bill of Materials
  - `CPL-ESP32S3_dual_ST7735_display.csv`: Component Placement List
  - `GERBER-ESP32S3_dual_ST7735_display.zip`: Gerber files for PCB fabrication

### Visual References

- **images/**  
  Reference images, including PCB layout and assembled board photos:
  - `7735_dual_display.jpg`: Photo of assembled dual display board

### Interactive Tools

- **pcb/**  
  PCB design files and interactive Bill of Materials (BOM):
  - [`ibom.html`](pcb/ibom.html): Interactive HTML BOM and PCB viewer (open in browser)
  - [`Readme.md`](pcb/Readme.md): Usage instructions for the interactive BOM

## Hardware Variants

### ESP32 Variant

- Compatible with standard ESP32 development boards
- 4MB Flash memory
- Standard GPIO pin mapping
- See pin configuration in [`docs/hardware connections.md`](../docs/hardware%20connections.md)

### ESP32-S3 Variant

- Compatible with ESP32-S3-N8R2 modules
- 8MB Flash memory + 2MB PSRAM
- Enhanced GPIO capabilities
- See pin configuration in [`docs/hardware connections.md`](../docs/hardware%20connections.md)

## Notes

- All hardware files are provided for reference and personal use only
- Manufacturing files are production-ready for PCB fabrication
- Both ESP32 and ESP32-S3 variants support the same display functionality
- This project is in active development and files may change at any time without notice. Use at your own risk and responsibility
