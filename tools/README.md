# ESP32 Dual Display Billboard - Build Tools & Utilities

This directory contains the build automation tools, scripts, and configuration files used for developing, building, and deploying the ESP32 Dual Display Billboard System.

## Overview

The tools directory provides a comprehensive build pipeline supporting:

- **Multi-platform builds** (ESP32, ESP32-S3 variants)
- **Automated version management** with build numbering
- **Web content processing** and optimization
- **Splash screen generation** for different display types
- **Firmware binary management** with descriptive naming
- **Development monitoring** and debugging utilities
- **Memory partition management** for different hardware configurations

## Directory Contents

### Python Build Scripts

| Script | Purpose | Dependencies | Usage |
|--------|---------|--------------|-------|
| **`generate_build_info.py`** | Version management & build numbering | `json`, `datetime` | Automatic (PlatformIO hook) |
| **`minify.py`** | Web content minification & compression | `htmlmin2`, `jsmin` | Manual execution |
| **`rename_firmware.py`** | Post-build firmware binary renaming | Built-in | Automatic (PlatformIO hook) |
| **`png_to_splash_header_converter.py`** | PNG to RGB565 header conversion | `Pillow` | Manual execution |
| **`generate_splash_screen.py`** | Smart splash screen generation | `subprocess` | Automatic (build detection) |
| **`ethernet_monitor.py`** | HTTP-based device monitoring | `requests` | Manual execution |

### Partition Tables

| File | Target Platform | Flash Size | Filesystem | Special Features |
|------|----------------|------------|------------|------------------|
| **`ESP32_4MB_1MB_FS.csv`** | ESP32 Classic | 4MB | 1MB SPIFFS | Standard dual-app OTA |
| **`ESP32S3_8MB_1MB_FS.csv`** | ESP32-S3 | 8MB | 1.4MB SPIFFS | Extended storage + coredump |
| **`ESP32S3_8MB_2MB_PSRAM.csv`** | ESP32-S3 + PSRAM | 8MB | Variable | PSRAM-optimized layout |

### Placeholder Files

| File | Status | Purpose |
|------|--------|---------|
| **`png_to_header.py`** | Empty | Reserved for future PNG conversion |
| **`png_to_splash_header.py`** | Empty | Legacy splash conversion (replaced) |

## Tool Descriptions

### 1. Build Information Generator (`generate_build_info.py`)

**Purpose**: Automated version management with daily build numbering

**Features**:

- Extracts base version from `platformio.ini`
- Generates unique daily build numbers
- Creates `include/version.h` and `data/build_info.json`
- Supports debug/production/testing build variants
- Maintains build consistency across compilation units

**Generated Files**:

```cpp
// include/version.h example
#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_BUILD 42
#define BUILD_DATE "2025-08-16"
```

### 2. Web Content Minifier (`minify.py`)

**Purpose**: Web asset optimization for embedded deployment

**Features**:

- HTML minification using `htmlmin2`
- JavaScript minification using `jsmin`
- GZIP compression for maximum space efficiency
- Preserves critical formatting while reducing size
- Batch processing of all web assets

**Usage**:

```bash
cd tools
python minify.py
```

**Dependencies**:

```bash
pip install htmlmin2 jsmin
```

### 3. Firmware Renamer (`rename_firmware.py`)

**Purpose**: Intelligent firmware binary management

**Features**:

- Descriptive firmware naming based on build environment
- Automatic debug/production/testing variant detection
- Hardware-specific naming (ESP32, ESP32-S3)
- Display driver identification (ST7735, ST7789)

**Output Examples**:

- `esp32_ST7735_debug.bin`
- `esp32s3_ST7789_production.bin`
- `esp32_ST7735_testing.bin`

### 4. PNG to Header Converter (`png_to_splash_header_converter.py`)

**Purpose**: Image conversion for embedded displays

**Features**:

- RGB888 to RGB565 color conversion
- Automatic image resizing for target displays
- C header file generation with embedded arrays
- Support for transparency and alpha channels

**Usage**:

```bash
python png_to_splash_header_converter.py input.png output.h
```

**Supported Displays**:

```bash
python generate_splash_screen.py  # Auto-detects environment
```

**Supported Displays**:

- **ST7735**: 80x160 or 160x80 pixels
- **ST7789**: 240x240 or 240x320 pixels

### 5. Smart Splash Screen Generator (`generate_splash_screen.py`)

**Purpose**: Context-aware splash screen creation

**Features**:

- Automatic build environment detection
- Display driver auto-detection from build flags
- Intelligent image resizing and optimization
- Integration with build pipeline

**Operation**:

```text
Input: PNG source + build environment
Process: Auto-resize + color conversion + header generation
Output: Ready-to-compile C header file
```

### 6. Ethernet Monitor (`ethernet_monitor.py`)

**Purpose**: Network-based device monitoring and debugging

**Features**:

- Multi-device monitoring support
- HTTP-based status queries
- Real-time performance metrics
- Network connectivity validation

**Configuration**:

```python
DEVICES = [
    {"name": "Billboard-01", "ip": "192.168.1.100"},
    {"name": "Billboard-02", "ip": "192.168.1.101"}
]
```

**Usage**:

```bash
python ethernet_monitor.py
```

## Partition Table Details

### ESP32 Classic (4MB Flash)

```text
app0:     1.5MB  (OTA Partition A)
app1:     1.5MB  (OTA Partition B)
spiffs:   1MB    (File System)
nvs:      24KB   (Non-Volatile Storage)
otadata:  8KB    (OTA Status)
```

### ESP32-S3 (8MB Flash)

```text
app0:     2.5MB  (OTA Partition A)
app1:     2.5MB  (OTA Partition B)
spiffs:   1.4MB  (Extended File System)
coredump: 64KB   (Debug Support)
nvs:      24KB   (Non-Volatile Storage)
```

### ESP32-S3 with PSRAM (8MB + 2MB PSRAM)

- Optimized partition layout for PSRAM utilization
- Enhanced memory allocation for large image buffers
- Extended filesystem for media storage

## Development Workflow

### 1. Initial Setup

```bash
# Install Python dependencies
pip install htmlmin2 jsmin Pillow requests

# Verify tools are accessible
cd tools
python --version  # Ensure Python 3.6+
```

### 2. Pre-Build Operations

```bash
# Minify web content before building
python minify.py

# Generate custom splash screens (if needed)
python png_to_splash_header_converter.py logo.png splash.h
```

### 3. Build Process

```bash
# Standard PlatformIO build (tools run automatically)
pio run

# Upload with monitoring
pio run --target upload --target monitor
```

### 4. Post-Build Verification

```bash
# Check firmware names in firmware/ directory
ls ../firmware/*.bin

# Monitor deployed devices
python ethernet_monitor.py  # Monitor network-connected devices
```

## Troubleshooting

### Common Issues

#### 1. Missing Python Dependencies

```bash
# Error: ModuleNotFoundError: No module named 'htmlmin2'
pip install htmlmin2 jsmin Pillow requests
```

#### 2. Build Info Generation Fails

```bash
# Check platformio.ini for version definitions
grep "version" ../platformio.ini
# Ensure write permissions to include/ directory
```

#### 3. PNG Conversion Errors

```bash
# Verify Pillow installation
python -c "from PIL import Image; print('Pillow OK')"
# Check image format and dimensions
```

#### 4. Partition Table Issues

```bash
# Verify partition table exists
ls ESP32*.csv
# Check PlatformIO environment configuration
```

### Environment Variables

Build scripts utilize these PlatformIO environment variables:

- `PIOENV`: Current PlatformIO environment
- `PIOPLATFORM`: Target platform (espressif32)
- `PIOHOME_DIR`: PlatformIO home directory

### Generated File Locations

- Build info: `include/version.h`
- Web assets: `data/*.gz` (compressed versions)
- Firmware: `firmware/*.bin` (renamed binaries)
- Splash screens: `include/splash_*.h`

## Integration Details

### Web Content Optimization

- **GZIP Compression**: Reduces web assets by ~70%
- **Minification**: Removes unnecessary whitespace and comments
- **Asset Bundling**: Combines multiple files where beneficial

### Memory Management

- **Partition Optimization**: Balanced OTA and storage allocation
- **PSRAM Integration**: Enhanced buffer management for S3 variants
- **Dynamic Allocation**: Runtime memory optimization

### Build Efficiency

- **Incremental Processing**: Only regenerates changed content
- **Parallel Execution**: Multi-threaded operations where possible
- **Caching Strategy**: Reuses unchanged assets
- **Dependency Tracking**: Smart rebuild triggering

### PlatformIO Integration

```ini
# Example platformio.ini integration
[env:esp32]
extra_scripts = 
    tools/generate_build_info.py
    tools/rename_firmware.py
board_build.partitions = tools/ESP32_4MB_1MB_FS.csv
```

### Source Code Integration

- **Version Macros**: Compile-time version information
- **Build Flags**: Environment-specific compilation options
- **Asset Embedding**: Automatic resource inclusion
- **OTA Updates**: Dual partition scheme

## Best Practices

### Tool Usage

1. **Run minify.py** after any web content changes
2. **Verify build numbers** increment correctly for releases
3. **Test partition layouts** on target hardware before deployment
4. **Monitor build sizes** to prevent partition overflow
5. **Use descriptive firmware names** for deployment tracking

### Security Considerations

1. **Exclude sensitive data** from version control
2. **Validate partition boundaries** before deployment
3. **Test OTA updates** thoroughly in staging environment
4. **Monitor memory usage** during development

### Performance Optimization

1. **Profile memory usage** with different partition layouts
2. **Benchmark web asset loading** after minification
3. **Test display performance** with generated splash screens
4. **Validate build times** with automated scripts
