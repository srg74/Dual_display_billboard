#!/usr/bin/env python3
"""
Smart splash screen generator for PlatformIO builds
Automatically selects the correct PNG file based on build environment
"""

import os
import sys
import subprocess

def detect_build_environment():
    """Detect if we're building for ST7735 or ST7789"""
    
    # Check PlatformIO environment variable
    pioenv = os.environ.get('PIOENV', '')
    
    if 'st7789' in pioenv.lower():
        return 'ST7789'
    elif 'st7735' in pioenv.lower():
        return 'ST7735'
    
    # Fallback: check build flags from command line
    for arg in sys.argv:
        if 'DISPLAY_TYPE_ST7789' in arg:
            return 'ST7789'
        elif 'DISPLAY_TYPE_ST7735' in arg:
            return 'ST7735'
    
    # Default fallback
    print("Warning: Could not detect display type, defaulting to ST7735")
    return 'ST7735'

def check_existing_splash_screen(output_file, png_file, display_type):
    """Check if the existing splash_screen.h is already correct"""
    
    if not os.path.exists(output_file):
        return False, "Output file doesn't exist"
    
    try:
        # Read the header comment to check what it was generated from
        with open(output_file, 'r') as f:
            content = f.read(500)  # Read first 500 chars to check header
        
        # Check if it mentions the correct PNG file
        png_filename = os.path.basename(png_file)
        if f"Generated from {png_filename}" not in content:
            return False, f"Generated from different source (not {png_filename})"
        
        # Check if it mentions the correct display type
        if f"Target display: {display_type}" not in content:
            return False, f"Generated for different display type (not {display_type})"
        
        # Check file timestamps - regenerate if PNG is newer than header
        png_mtime = os.path.getmtime(png_file)
        header_mtime = os.path.getmtime(output_file)
        
        if png_mtime > header_mtime:
            return False, "PNG file is newer than header file"
        
        return True, "Existing file is up to date"
        
    except Exception as e:
        return False, f"Error checking file: {e}"

def main():
    """Generate splash_screen.h based on build environment"""
    
    # Get project root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    # Detect display type
    display_type = detect_build_environment()
    print(f"🎯 Detected display type: {display_type}")
    
    # Select appropriate PNG file
    if display_type == 'ST7789':
        png_file = os.path.join(project_root, 'test_images', 'splash_screen_240X240.png')
    else:  # ST7735
        png_file = os.path.join(project_root, 'test_images', 'splash_screen_80X160.png')
    
    # Output file
    output_file = os.path.join(project_root, 'include', 'splash_screen.h')
    
    # Check if PNG file exists
    if not os.path.exists(png_file):
        print(f"❌ Error: PNG file not found: {png_file}")
        sys.exit(1)
    
    # Check if the existing splash screen is already correct
    is_current, reason = check_existing_splash_screen(output_file, png_file, display_type)
    
    if is_current:
        print(f"✅ Splash screen already up to date for {display_type}")
        print(f"   📁 Using existing: {os.path.basename(output_file)}")
        return  # Skip regeneration
    else:
        print(f"🔄 Regenerating splash screen: {reason}")
    
    # Run the conversion script
    converter_script = os.path.join(script_dir, 'png_to_splash_header_converter.py')
    
    print(f"🔄 Converting {os.path.basename(png_file)} → splash_screen.h")
    
    try:
        result = subprocess.run([
            sys.executable, converter_script, png_file, output_file, display_type
        ], capture_output=True, text=True, check=True)
        
        print(f"✅ Successfully generated splash_screen.h for {display_type}")
        if result.stdout:
            print(result.stdout)
            
    except subprocess.CalledProcessError as e:
        print(f"❌ Error generating splash screen: {e}")
        if e.stdout:
            print(f"STDOUT: {e.stdout}")
        if e.stderr:
            print(f"STDERR: {e.stderr}")
        sys.exit(1)

if __name__ == "__main__":
    main()
