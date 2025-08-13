#!/usr/bin/env python3
"""
Pre-build script to generate build date and inject it into build flags
Generates YYMMDD format date for build identification
"""

try:
    Import("env")
except:
    pass

from datetime import datetime

def generate_build_date(source, target, env):
    """Generate build date in YYMMDD format"""
    
    # Generate current date in YYMMDD format
    build_date = datetime.now().strftime("%y%m%d")
    
    # Add build date to build flags
    env.Append(CPPDEFINES=[
        ("BUILD_DATE", f'\\"{build_date}\\"')
    ])
    
    print(f"🗓️  Build Date: {build_date}")

# Execute before building
if 'env' in globals():
    env.AddPreAction("buildprog", generate_build_date)
