# Dual Display Billboard System 🎯

**Professional ESP32/ESP32-S3 Digital Billboard with Advanced Image Management**

A sophisticated dual display billboard system designed for scale modelers, featuring WiFi connectivity, image slideshow capabilities, multiple clock faces, and professional-grade code architecture.

## ✨ Key Features

### 🖥️ **Dual Display Support**

- Independent control of two ST7735 (160x80) displays
- ST7789 (240x240) compatibility 
- Hardware-specific pin mapping for ESP32 and ESP32-S3
- Content-aware rotation settings

### 📸 **Advanced Image Management**

- **10-image maximum limit** for optimal memory management
- JPEG validation with dimension checking
- 50KB per image size limit
- Automatic storage monitoring
- User-friendly error reporting

### 🕐 **Multiple Clock Faces**

- **Modern Square**: Rounded border analog design with colored hands
- **Classic Analog**: Traditional analog clock with hour markers
- **Digital Modern**: Large font digital display
- **Minimalist**: Clean, simple time display

### 🌐 **WiFi & Web Management**

- Captive portal setup for easy configuration
- WiFi network scanning and connection
- Web-based image upload interface
- Real-time system status monitoring

### 🔧 **Professional Code Architecture**

- Comprehensive documentation with Doxygen-style comments
- Configurable logging system (ERROR/WARN/INFO levels)
- Memory management and error handling
- Modular component design
- Production-ready code standards

## 📚 Documentation

This project features comprehensive documentation generated from Doxygen-style annotations:

### 🏗️ **Code Documentation**

- **235+ documented methods** across 13 implementation files
- **19 fully documented header files** with complete API reference
- **Professional Doxygen formatting** with @brief, @param, @return tags
- **Cross-references and usage examples** throughout

### 🌐 **Web-Based Documentation (Apple Silicon Compatible)**

Perfect for M1/M2/M3 Macs - no local installation required!

```bash
# Generate documentation using GitHub Actions
./generate_docs_web.sh

# Opens documentation portal in browser
open docs/documentation-portal.html
```

### 📖 **Access Methods**

- **GitHub Actions**: Download documentation artifacts
- **GitHub Pages**: Live documentation at `your-repo/api-docs/`
- **Local Portal**: Open `docs/documentation-portal.html`

### 📂 **Documentation Structure**

```text
docs/
├── generated/           # Auto-generated API docs (GitHub Actions)
│   └── html/           # Professional HTML documentation
├── documentation-portal.html  # Web-based documentation hub
├── DOCUMENTATION.md    # Complete setup guide
└── hardware connections.md
```
