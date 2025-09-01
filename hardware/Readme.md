# Hardware Folder Overview

This folder contains all hardware-related resources for the ST7735 Dual Display Billboard project.

## Contents

### Manufacturing Files

- **ESP32/**  
  Production files for ESP32 variant:
  - `BOM-ESP32_dual_ST7735_display.csv`: Bill of Materials
  - `CPL-ESP32_dual_ST7735_display.csv`: Component Placement List
  - `ESP32 dual ST7735 display controller.pdf`: Complete circuit schematic
  - `GERBER-ESP32_dual_ST7735_display.zip`: Gerber files for PCB fabrication
  - `images/`: PCB layout reference images
  - `pcb/`: Interactive BOM and PCB viewer files

- **ESP32S3/**  
  Production files for ESP32-S3 variant:
  - `BOM-ESP32S3_dual_ST7735_display.csv`: Bill of Materials
  - `CPL-ESP32S3_dual_ST7735_display.csv`: Component Placement List
  - `ESP32S3 dual ST7735 display controller.pdf`: Complete circuit schematic
  - `GERBER-ESP32S3_dual_ST7735_display.zip`: Gerber files for PCB fabrication

### Visual References

- **ESP32/images/**  
  Reference images for ESP32 variant, including PCB layout:
  - `ESP32_dual_ST7735_display.png`: PCB layout image for ESP32 dual display board

### Interactive Tools

- **ESP32/pcb/**  
  PCB design files and interactive Bill of Materials (BOM) for ESP32 variant:
  - [`ibom.html`](ESP32/pcb/ibom.html): Interactive HTML BOM and PCB viewer (open in browser)
  - [`Readme.md`](ESP32/pcb/Readme.md): Usage instructions for the interactive BOM

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
- The ESP32 variant includes additional resources (images folder and interactive PCB tools)
- The ESP32-S3 variant focuses on essential manufacturing files
- This project is in active development and files may change at any time without notice. Use at your own risk and responsibility
