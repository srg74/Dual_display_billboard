#!/usr/bin/env python3
"""
PNG to Splash Screen Header Converter
Converts PNG files to RGB565 format for ST7735/ST7789 TFT displays

Usage:
    python png_to_splash_header_converter.py input.png output.h [display_type]

Arguments:
    input.png    - Source PNG file
    output.h     - Output header file
    display_type - ST7735 (80x160) or ST7789 (240x240) [default: auto-detect from image size]

Requirements:
    pip install Pillow
"""

import sys
import os
from PIL import Image
import argparse

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565 format"""
    # Convert 8-bit values to 5-bit (red), 6-bit (green), 5-bit (blue)
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F  
    b5 = (b >> 3) & 0x1F
    
    # Pack into 16-bit value: RRRRRGGG GGGBBBBB
    return (r5 << 11) | (g6 << 5) | b5

def detect_display_type(width, height):
    """Auto-detect display type based on image dimensions"""
    if width == 80 and height == 160:
        return "ST7735"
    elif width == 240 and height == 240:
        return "ST7789"
    elif width == 160 and height == 80:
        return "ST7735_ROTATED"
    else:
        return "CUSTOM"

def convert_png_to_header(input_file, output_file, display_type=None):
    """Convert PNG file to RGB565 header file"""
    
    try:
        # Open and process the image
        print(f"Opening image: {input_file}")
        image = Image.open(input_file)
        
        # Convert to RGB if necessary
        if image.mode != 'RGB':
            print(f"Converting from {image.mode} to RGB")
            image = image.convert('RGB')
        
        width, height = image.size
        print(f"Image size: {width}x{height}")
        
        # Auto-detect display type if not specified
        if display_type is None:
            display_type = detect_display_type(width, height)
            print(f"Auto-detected display type: {display_type}")
        
        # Validate dimensions
        expected_dims = {
            "ST7735": [(80, 160), (160, 80)],
            "ST7789": [(240, 240)],
            "ST7735_ROTATED": [(160, 80)],
            "CUSTOM": []
        }
        
        if display_type in expected_dims and expected_dims[display_type]:
            if (width, height) not in expected_dims[display_type]:
                print(f"Warning: Unexpected dimensions for {display_type}")
                print(f"Expected: {expected_dims[display_type]}, got: ({width}, {height})")
        
        # Get pixel data
        pixels = list(image.getdata())
        total_pixels = len(pixels)
        total_bytes = total_pixels * 2  # 2 bytes per pixel for RGB565
        
        print(f"Total pixels: {total_pixels}")
        print(f"Output size: {total_bytes} bytes")
        
        # Generate header file
        print(f"Generating header file: {output_file}")
        
        with open(output_file, 'w') as f:
            # Write header comments
            f.write("/*\n")
            f.write(" * Splash screen color bitmap header file\n")
            f.write(f" * Generated from {os.path.basename(input_file)}\n")
            f.write(f" * Target display: {display_type}\n")
            f.write(f" * Image size: {width}x{height} pixels\n")
            f.write(f" * RGB565 color bitmap ({width}x{height}, {total_bytes} bytes)\n")
            f.write(" * Generated with png_to_splash_header_converter.py\n")
            f.write(" */\n\n")
            
            # Write header guards
            f.write("#ifndef SPLASH_SCREEN_H\n")
            f.write("#define SPLASH_SCREEN_H\n\n")
            f.write("#include <Arduino.h>\n\n")
            
            # Write dimensions
            f.write(f"// Bitmap dimensions for {display_type}\n")
            f.write(f"#define SPLASH_WIDTH  {width}\n")
            f.write(f"#define SPLASH_HEIGHT {height}\n")
            f.write(f"#define SPLASH_SIZE   {total_bytes}\n\n")
            
            # Write bitmap data
            f.write("// Splash screen color bitmap data (RGB565)\n")
            f.write(f"// Compatible with {display_type} ({width}x{height})\n")
            f.write("const uint16_t epd_bitmap_[] PROGMEM = {\n")
            
            # Convert pixels to RGB565 and write data
            pixels_per_line = 8  # 8 hex values per line for readability
            for i, pixel in enumerate(pixels):
                if isinstance(pixel, tuple) and len(pixel) >= 3:
                    r, g, b = pixel[0], pixel[1], pixel[2]
                else:
                    # Handle grayscale or other formats
                    r = g = b = pixel if isinstance(pixel, int) else pixel[0]
                
                rgb565 = rgb888_to_rgb565(r, g, b)
                
                # Format output
                if i % pixels_per_line == 0:
                    f.write("  ")
                
                f.write(f"0x{rgb565:04X}")
                
                if i < total_pixels - 1:
                    f.write(", ")
                
                if (i + 1) % pixels_per_line == 0:
                    f.write("\n")
            
            # Close the array and header
            if total_pixels % pixels_per_line != 0:
                f.write("\n")
            f.write("};\n\n")
            f.write("#endif // SPLASH_SCREEN_H\n")
        
        print(f" Successfully generated {output_file}")
        print(f" Array size: {total_pixels} uint16_t values ({total_bytes} bytes)")
        
        return True
        
    except Exception as e:
        print(f" Error: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description="Convert PNG files to RGB565 header files for TFT displays",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python png_to_splash_header_converter.py splash_80x160.png splash_screen.h
  python png_to_splash_header_converter.py splash_240x240.png splash_screen.h ST7789
  
Supported display types:
  ST7735      - 80x160 or 160x80 pixels
  ST7789      - 240x240 pixels
  CUSTOM      - Any size (auto-detected)
        """
    )
    
    parser.add_argument('input', help='Input PNG file')
    parser.add_argument('output', help='Output header file (.h)')
    parser.add_argument('display_type', nargs='?', default=None,
                       choices=['ST7735', 'ST7789', 'CUSTOM'],
                       help='Target display type (auto-detected if not specified)')
    
    args = parser.parse_args()
    
    # Validate input file
    if not os.path.exists(args.input):
        print(f" Error: Input file '{args.input}' not found")
        return 1
    
    # Ensure output directory exists
    output_dir = os.path.dirname(args.output)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Convert the file
    success = convert_png_to_header(args.input, args.output, args.display_type)
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
