#!/usr/bin/env python3
"""
Post-build script to rename and archive PRODUCTION firmware binaries only
Creates firmware files with format: platform_display_production.bin
Examples: esp32_ST7735_production.bin, esp32s3_ST7735_production.bin

Debug builds are not archived - they remain in .pio/build/ for temporary use
Production builds are copied to firmware/ folder for distribution and OTA updates
"""

try:
    Import("env")
except:
    pass

import os
import shutil

def rename_firmware(source, target, env):
    """Rename firmware binary to descriptive name - PRODUCTION BUILDS ONLY"""
    
    # Get environment name to determine platform and build type
    env_name = env["PIOENV"]
    
    # Only process production builds
    if "production" not in env_name:
        print(f" Debug build detected ({env_name}) - skipping firmware archival")
        print(f" Only production builds are copied to firmware/ folder")
        return
    
    # Parse environment name to extract info
    platform = "unknown"
    display = "unknown" 
    build_type = "production"  # We know it's production at this point
    
    if "esp32dev" in env_name:
        platform = "esp32"
    elif "esp32s3" in env_name:
        platform = "esp32s3"
    
    if "st7735" in env_name:
        display = "ST7735"
    elif "st7789" in env_name:
        display = "ST7789"
    
    # Create descriptive filename (no date stamp in filename)
    firmware_name = f"{platform}_{display}_{build_type}.bin"
    
    # Get source firmware path
    source_path = str(target[0])
    
    # Create firmware directory if it doesn't exist
    firmware_dir = os.path.join(env["PROJECT_DIR"], "firmware")
    os.makedirs(firmware_dir, exist_ok=True)
    
    # Destination path
    dest_path = os.path.join(firmware_dir, firmware_name)
    
    try:
        # Copy firmware to descriptive name
        shutil.copy2(source_path, dest_path)
        
        print(f" Firmware copied to: firmware/{firmware_name}")
        print(f" Ready for OTA upload: {firmware_name}")
        
    except Exception as e:
        print(f" Error copying firmware: {e}")

# Add the post-build action
if 'env' in globals():
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", rename_firmware)
