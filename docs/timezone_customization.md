# Timezone Configuration Guide

This document explains how to customize the timezone options available in the web interface.

## Overview

The dual display billboard system uses the `include/timezone_config.h` file to define available timezone options. This approach allows users to easily add or modify timezone choices without touching the core time management code.

## Adding Custom Timezones

### Step 1: Edit the Configuration File

Open `include/timezone_config.h` and locate the "USER CUSTOMIZATION AREA" (around line 62).

### Step 2: Add Your Timezone Entry

Add a new entry to the `TIMEZONE_OPTIONS` array following this pattern:

```cpp
{"POSIX_TZ_STRING", "Display Name"},
```

### Example Additions

Here are some common timezone examples you can add:

```cpp
// Asia
{"IST-5:30", "India Standard Time"},
{"CST-8", "China Standard Time"},
{"JST-9", "Japan Standard Time"},

// Europe
{"MSK-3", "Moscow Time"},
{"WET0WEST,M3.5.0/1,M10.5.0", "Western Europe"},

// Americas
{"BRT3BRST,M10.3.0/0,M2.3.0/0", "Brazil Eastern"},
{"ART3", "Argentina Time"},

// Oceania
{"NZST-12NZDT,M9.5.0,M4.1.0/3", "New Zealand"},
```

### Step 3: Recompile and Upload

After adding your custom timezones:

1. Save the file
2. Recompile the firmware using PlatformIO
3. Upload to your device

## POSIX TZ Format Reference

The timezone strings use POSIX TZ format:

### Basic Format

`STD[offset[DST[offset],start[/time],end[/time]]]`

### Components

- **STD**: Standard time zone abbreviation (e.g., EST, CET)
- **offset**: Hours behind UTC (positive for west of Greenwich)
- **DST**: Daylight saving time abbreviation (optional)
- **start/end**: DST start/end rules (optional)

### DST Rules Format

- `M3.5.0`: 5th occurrence of day 0 (Sunday) in month 3 (March)
- `M10.5.0/3`: Same as above, but transition at 3:00 AM

### Examples

- `"UTC0"` - UTC timezone (no DST)
- `"EST5EDT,M3.2.0,M11.1.0"` - US Eastern Time
- `"CET-1CEST,M3.5.0,M10.5.0/3"` - Central European Time
- `"JST-9"` - Japan Standard Time (no DST)

## Finding Your Timezone

### Online Resources

1. [POSIX TZ Database](https://en.wikipedia.org/wiki/Tz_database)
2. [TimeZone Converter](https://www.timeanddate.com/time/zones/)

### Testing Your Timezone

After adding a custom timezone:

1. Access the web interface
2. Go to Settings
3. Check if your timezone appears in the dropdown
4. Select it and verify the time displays correctly

## Troubleshooting

### Timezone Not Appearing

- Check syntax in `timezone_config.h`
- Ensure comma after each entry except the last
- Verify the file was saved before recompiling

### Incorrect Time Display

- Verify the POSIX TZ string is correct
- Check DST rules for your location
- Test with a known working timezone first

### Build Errors

- Check for syntax errors (missing commas, quotes)
- Ensure the header file is properly formatted
- Verify no duplicate entries

## Notes

- Changes require recompilation and firmware upload
- The current timezone setting is stored in device memory
- Maximum timezone string length is typically 64 characters
- DST rules follow local regulations (verify accuracy for your area)

## Support

If you encounter issues with timezone configuration, check:

1. Syntax of your timezone string
2. Build logs for compilation errors
3. Device logs for runtime errors

Remember to backup your custom configuration before firmware updates.
