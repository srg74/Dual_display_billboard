#!/usr/bin/env python3
"""
🔧 Enhanced Build Information Generator for ESP32 Dual Display Billboard
@version 0.9
@author Dual Display Billboard Team
@date 2025-08-16

Generates comprehensive build information including:
• Base version from platformio.ini (e.g., v0.9)
• Build number in YYMMDDx format (e.g., 2508160)
• Full version string (e.g., v0.9-build.2508160)
• Build timestamp and environment information
"""

try:
    Import("env")
except:
    pass

import os
import json
from datetime import datetime

class BuildInfoGenerator:
    """
    📦 Comprehensive build information generator with version management
    
    Features:
    • Base version extraction from platformio.ini
    • Daily build counter with automatic increment
    • YYMMDDx format build numbers (x = daily build 0-9)
    • Persistent build counter storage
    • Multiple version format generation
    """
    
    def __init__(self, env):
        self.env = env
        self.build_info_file = "build_info.json"
        self.base_version = self._get_base_version()
        
    def _get_base_version(self):
        """Extract base version from platformio.ini"""
        try:
            # Try to get from environment current_version
            # For now, let's use the VERSION define that's already set in build_flags
            return self.env.GetProjectOption("current_version", "0.9")
        except:
            return "0.9"  # Fallback version
    
    def _load_build_info(self):
        """Load existing build information from JSON file"""
        if os.path.exists(self.build_info_file):
            try:
                with open(self.build_info_file, 'r') as f:
                    return json.load(f)
            except:
                pass
        return {}
    
    def _save_build_info(self, build_info):
        """Save build information to JSON file"""
        try:
            with open(self.build_info_file, 'w') as f:
                json.dump(build_info, f, indent=2)
        except Exception as e:
            print(f"⚠️  Warning: Could not save build info: {e}")
    
    def _generate_build_number(self):
        """
        Generate build number in YYMMDDx format
        x = daily build counter (0-9, resets daily)
        """
        now = datetime.now()
        date_part = now.strftime("%y%m%d")  # YYMMDD
        
        # Load existing build info
        build_info = self._load_build_info()
        
        # Check if it's a new day or first build
        last_date = build_info.get("last_date", "")
        daily_counter = build_info.get("daily_counter", 0)
        
        if last_date != date_part:
            # New day - reset counter
            daily_counter = 0
        else:
            # Same day - increment counter
            daily_counter += 1
            
        # Ensure counter stays within 0-9
        if daily_counter > 9:
            daily_counter = 9
            print(f"⚠️  Warning: Daily build limit reached (9), keeping counter at 9")
        
        # Generate build number
        build_number = f"{date_part}{daily_counter}"
        
        # Update and save build info
        build_info.update({
            "last_date": date_part,
            "daily_counter": daily_counter,
            "last_build": build_number,
            "build_timestamp": now.isoformat()
        })
        self._save_build_info(build_info)
        
        return build_number, daily_counter
    
    def generate(self):
        """Generate comprehensive build information"""
        build_number, daily_counter = self._generate_build_number()
        
        # Generate version strings
        full_version = f"v{self.base_version}-build.{build_number}"
        version_short = f"v{self.base_version}"
        
        # Generate build timestamp
        build_timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        build_date = datetime.now().strftime("%y%m%d")
        
        # Environment information
        env_name = self.env.get("PIOENV", "unknown")
        
        # Add all version information to build flags
        self.env.Append(CPPDEFINES=[
            ("FIRMWARE_VERSION", f'\\"{version_short}\\"'),
            ("FIRMWARE_VERSION_FULL", f'\\"{full_version}\\"'),
            ("BUILD_NUMBER", f'\\"{build_number}\\"'),
            ("BUILD_DATE", f'\\"{build_date}\\"'),
            ("BUILD_TIMESTAMP", f'\\"{build_timestamp}\\"'),
            ("BUILD_ENVIRONMENT", f'\\"{env_name}\\"'),
            ("DAILY_BUILD_COUNT", str(daily_counter))
        ])
        
        # Print build information
        print(f"🏗️  Build Information Generated:")
        print(f"   📦 Base Version: v{self.base_version}")
        print(f"   � Build Number: {build_number}")
        print(f"   📅 Build Date: {build_date}")
        print(f"   🕐 Daily Build: #{daily_counter}")
        print(f"   ✅ Full Version: {full_version}")
        print(f"   🏷️  Environment: {env_name}")

def generate_build_info(source, target, env):
    """Generate build information for the project"""
    print("🏗️  Build Info Generator Starting...")
    try:
        generator = BuildInfoGenerator(env)
        generator.generate()
        print("✅ Build info generated successfully!")
    except Exception as e:
        print(f"❌ Build info generation failed: {e}")

# Execute before building
if 'env' in globals():
    print("📦 Registering build info generator...")
    env.AddPreAction("buildprog", generate_build_info)
