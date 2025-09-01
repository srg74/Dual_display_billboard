# ST7735 Vertical Breakout Board

This folder contains the design files and manufacturing resources for the ST7735 vertical orientation breakout board.

## Overview

The ST7735 vertical breakout board is designed to provide a clean, reliable interface for ST7735 TFT LCD displays in a vertical mounting configuration. This board simplifies connections and provides mechanical stability for display modules.

## Files Included

### Manufacturing Files

- **`GERBER-ST7735_break_out_board_vertical.zip`**  
  Complete Gerber files ready for PCB fabrication, including:
  - Copper layers
  - Solder mask layers
  - Silkscreen layers
  - Drill files
  - Pick and place files

### Documentation

- **`ST7735_break_out_board_vertical.pdf`**  
  Complete circuit schematic and PCB layout documentation

### Visual References

- **`ST7735_break_out_board_vertical.png`**  
  PCB layout image showing component placement and routing

## Board Specifications

### Physical Characteristics

- **Orientation**: Vertical mounting configuration
- **Connector Type**: Standard 2.54mm pitch headers
- **PCB Thickness**: Standard 1.0mm FR4
- **Dimensions**: Optimized for ST7735 display modules

### Electrical Features

- **Signal Routing**: Optimized trace layout for signal integrity
- **Power Distribution**: Dedicated power and ground planes
- **Connector Pinout**: Standard ST7735 pin assignments
- **Voltage Compatibility**: 3.3V logic levels

## Supported Display Types

This breakout board is compatible with:

- ST7735 80x160 TFT LCD displays
- SPI interface displays with similar pinouts
- [ST7735 display data sheet used in design](Newvisio-N096-1608TBBIG11-H13_C2890616.pdf)

## Usage

1. **Fabrication**: Use the Gerber files for PCB manufacturing
2. **Assembly**: Solder headers and any required components
3. **Connection**: Connect to your ST7735 display module
4. **Integration**: Interface with ESP32/ESP32-S3 controller boards

## Pin Configuration

For detailed pin mapping and wiring instructions, refer to:

- [`../../../docs/hardware connections.md`](../../../docs/hardware%20connections.md)

## Manufacturing Notes

- Standard PCB fabrication process (no special requirements)
- Lead-free solder compatible
- RoHS compliant design
- Suitable for both hand soldering and automated assembly

## Design Files

The original design files are maintained in KiCAD format. The exported files in this folder represent the current production version.

## Notes

- All files are provided for reference and personal use only
- Manufacturing files are production-ready for PCB fabrication
- This design is optimized for cost-effective fabrication
- This project is in active development and files may change at any time without notice. Use at your own risk and responsibility

## Related Resources

- [Main Hardware Documentation](../../Readme.md)
- [Display Boards Overview](../Readme.md)
- [Project Documentation](../../../docs/)
