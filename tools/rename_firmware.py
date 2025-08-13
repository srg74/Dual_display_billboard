#!/usr/bin/env python3
"""
Post-build script to rename firmware binaries with descriptive names
Creates firmware files with format: platform_display_buildtype.bin
Examples: esp32_ST7735_debug.bin, esp32s3_ST7735_production.bin
"""

try:
    Import("env")
except:
    pass

import os
import shutil

def rename_firmware(source, target, env):
    """Rename firmware binary to descriptive name"""
    
    # Get environment name to determine platform and build type
    env_name = env["PIOENV"]
    
    # Parse environment name to extract info
    platform = "unknown"
    display = "unknown" 
    build_type = "unknown"
    
    if "esp32dev" in env_name:
        platform = "esp32"
    elif "esp32s3" in env_name:
        platform = "esp32s3"
    
    if "st7735" in env_name:
        display = "ST7735"
    elif "st7789" in env_name:
        display = "ST7789"
    
    if "debug" in env_name:
        build_type = "debug"
    elif "production" in env_name:
        build_type = "production"
    
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
        
        print(f"✅ Firmware copied to: firmware/{firmware_name}")
        print(f"📁 Ready for OTA upload: {firmware_name}")
        
        # Also create a latest symlink for convenience
        latest_name = f"{platform}_{display}_latest.bin"
        latest_path = os.path.join(firmware_dir, latest_name)
        
        # Remove existing symlink if it exists
        if os.path.exists(latest_path):
            os.remove(latest_path)
        
        # Create new symlink (or copy on Windows)
        try:
            os.symlink(firmware_name, latest_path)
        except OSError:
            # Fallback to copy on Windows
            shutil.copy2(dest_path, latest_path)
        
        print(f"🔗 Latest version: firmware/{latest_name}")
        
    except Exception as e:
        print(f"❌ Error copying firmware: {e}")

# Add the post-build action
if 'env' in globals():
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", rename_firmware)
