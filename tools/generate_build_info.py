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

import os
import json
from datetime import datetime

# Import PlatformIO environment
try:
    Import("env")
    ENV_AVAILABLE = True
except NameError:
    ENV_AVAILABLE = False
    env = None

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
            # Get the base version from the common section in platformio.ini
            return "0.9"  # Set to current version, can be made dynamic later
        except:
            return "0.9"  # Fallback version
            
    def _get_production_build_number(self):
        """Extract manual production build number from platformio.ini"""
        try:
            project_dir = self.env.get("PROJECT_DIR", os.getcwd())
            config_path = os.path.join(project_dir, 'platformio.ini')
            
            with open(config_path, 'r') as f:
                content = f.read()
                
            # Look for production_build = xxxxxxx in [common] section
            import re
            match = re.search(r'production_build\s*=\s*(\d+)', content)
            if match:
                return match.group(1)
        except Exception as e:
            print(f"⚠️  Could not read production_build from platformio.ini: {e}")
        
        # Fallback to date-based number
        now = datetime.now()
        return now.strftime("%y%m%d") + "0"
    
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
        For production builds: Use manual production_build number from platformio.ini
        For debug builds: Generate date-based number (but debug builds shouldn't call this script)
        """
        # Get environment name to determine if this is production
        env_name = self.env.get("PIOENV", "unknown")
        
        if "production" in env_name.lower():
            # Production build: Use manual number from platformio.ini
            build_number = self._get_production_build_number()
            print(f"📦 Using manual production build number: {build_number}")
            return build_number, 0  # No daily counter for manual builds
        else:
            # Debug build: Use original automatic logic (but this shouldn't happen)
            print(f"⚠️  Warning: Debug build calling generate_build_info.py - this should not happen")
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
        
        # Determine build type from environment name
        if "debug" in env_name.lower():
            build_type = "debug"
        elif "production" in env_name.lower():
            build_type = "production"
        else:
            build_type = "latest"
        
        # Update config.h file with current build information
        self._update_config_header(version_short, build_type, build_number)
        
        # Also add all version information to build flags for compatibility
        self.env.Append(CPPDEFINES=[
            ("FIRMWARE_VERSION", f'\\"{version_short}\\"'),
            ("FIRMWARE_VERSION_FULL", f'\\"{full_version}\\"'),
            ("BUILD_NUMBER", f'\\"{build_number}\\"'),
            ("BUILD_DATE", f'\\"{build_date}\\"'),
            ("BUILD_TYPE", f'\\"{build_type}\\"'),
            ("BUILD_TIMESTAMP", f'\\"{build_timestamp}\\"'),
            ("BUILD_ENVIRONMENT", f'\\"{env_name}\\"'),
            ("DAILY_BUILD_COUNT", str(daily_counter))
        ])
        
        # Print build information
        print(f"🏗️  Build Information Generated:")
        print(f"   📦 Version: {version_short}")
        print(f"   🔧 Build Type: {build_type}")
        print(f"   🏗️  Build: {build_number}")
        print(f"   📅 Build Date: {build_date}")
        print(f"   🕐 Daily Build: #{daily_counter}")
        print(f"   ✅ Full Version: {full_version}")
        print(f"   🏷️  Environment: {env_name}")
    
    def _update_config_header(self, version, build_type, build_number):
        """Update the config.h file with current build information"""
        # Get the project directory from PlatformIO environment
        project_dir = self.env.get("PROJECT_DIR", os.getcwd())
        config_path = os.path.join(project_dir, 'include', 'config.h')
        
        try:
            # Read the current config.h file
            with open(config_path, 'r') as f:
                content = f.read()
            
            # Update the build constants
            import re
            
            # Update BUILD_DATE with the full build number
            content = re.sub(
                r'#define BUILD_DATE "[^"]*"',
                f'#define BUILD_DATE "{build_number}"',
                content
            )
            
            # Update FIRMWARE_VERSION  
            content = re.sub(
                r'#define FIRMWARE_VERSION "[^"]*"',
                f'#define FIRMWARE_VERSION "{version}"',
                content
            )
            
            # Update BUILD_TYPE in the debug section
            content = re.sub(
                r'#define BUILD_TYPE "debug"',
                f'#define BUILD_TYPE "{build_type}"',
                content
            )
            
            # Write the updated content back
            with open(config_path, 'w') as f:
                f.write(content)
                
            print(f"📝 Updated config.h: {version}, {build_type}, {build_number}")
            
        except Exception as e:
            print(f"⚠️  Warning: Could not update config.h: {e}")

def generate_build_info(source, target, env):
    """Generate build information for the project"""
    print("🏗️  Build Info Generator Starting...")
    try:
        generator = BuildInfoGenerator(env)
        generator.generate()
        print("✅ Build info generated successfully!")
    except Exception as e:
        print(f"❌ Build info generation failed: {e}")

# Execute during PlatformIO build process
try:
    Import("env")
    print("📦 Registering build info generator...")
    env.AddPreAction("buildprog", generate_build_info)
except NameError:
    # Not running in PlatformIO environment
    pass
