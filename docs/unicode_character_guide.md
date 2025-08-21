# Unicode Character Support - User Guide

## Overview

The Dual Display Billboard system supports international characters through a custom Unicode rendering system. This guide explains how to add support for new characters when needed.

## Current Character Support

### Built-in Characters
- **English**: Full ASCII character set (a-z, A-Z, 0-9, punctuation)
- **German**: Umlauts (ü, ä, ö, Ü, Ä, Ö, ß)
- **Ready for expansion**: French, Spanish, Ukrainian Cyrillic, and other character sets

### Character Input
Users can enter international characters through the web interface:
1. Open device web interface in browser
2. Navigate to "Clock label" text field
3. Type any international characters (e.g., "Büro-Uhr", "Café", "Reloj")
4. Characters are automatically saved and displayed

## Adding New Characters

When users need characters not currently supported, follow these steps:

### Step 1: Identify Required Characters
Determine which specific characters are needed. Examples:
- **French**: é, è, à, ç, ê, â, î, ô, û
- **Spanish**: ñ, Ñ, á, é, í, ó, ú
- **Ukrainian Cyrillic**: А, Б, В, Г, Д, Е, Є, Ж, З, И, І, Ї, Й, К, Л, М, Н, О, П, Р, С, Т, У, Ф, Х, Ц, Ч, Ш, Щ, Ь, Ю, Я

### Step 2: Create Character Bitmaps

#### Tools Needed
- Bitmap font editor (e.g., FontForge, GIMP, or online pixel editors)
- Character bitmap must be 8x11 pixels for lowercase, 8x13 for uppercase
- 1-bit depth (black/white pixels)

#### Bitmap Format
Each character needs a bitmap array. Example for 'é':
```cpp
// Character 'é' (U+00E9) - 8x11 pixels
static const uint8_t glyph_e_acute[] = {
    0x18, 0x0C, 0x00,  // Accent mark: **... .**.. ....
    0x3C, 0x66, 0x7E,  // Letter body: .****. .**..** .*****. 
    0x60, 0x66, 0x3C   // Letter body: .**.... .**..** ..****..
};
```

#### Creating Bitmaps
1. **Draw character** in 8x11 pixel grid
2. **Convert to hex bytes** - each row becomes a hex value
3. **Test visual appearance** - ensure readability at small size

### Step 3: Add to Glyph Table

Edit `/include/text_utils.h` and `/src/text_utils.cpp`:

#### In text_utils.cpp, add to unicode_glyphs array:
```cpp
static const UnicodeGlyph unicode_glyphs[] = {
    // Existing German characters...
    {0x00FC, 8, 11, 0, 2, 8, glyph_u_umlaut},        // ü
    {0x00E4, 8, 11, 0, 2, 8, glyph_a_umlaut},        // ä
    {0x00F6, 8, 11, 0, 2, 8, glyph_o_umlaut},        // ö
    
    // Add new characters here:
    {0x00E9, 8, 11, 0, 2, 8, glyph_e_acute},         // é (French)
    {0x00F1, 8, 11, 0, 2, 8, glyph_n_tilde},         // ñ (Spanish)
    {0x0413, 8, 11, 0, 2, 8, glyph_cyrillic_ge},     // Г (Ukrainian)
};
```

#### Character Details
- **Unicode codepoint**: Get from Unicode tables (e.g., U+00E9 for é)
- **Dimensions**: width=8, height=11 for lowercase; height=13 for uppercase
- **Positioning**: xOffset=0, yOffset=2 for proper baseline alignment
- **Advance**: xAdvance=8 for standard character spacing

### Step 4: Test Implementation

#### Build and Upload
```bash
platformio run --target upload --environment esp32dev-st7735-debug
```

#### Test Characters
1. Open web interface
2. Set clock label with new characters
3. Verify display shows characters correctly
4. Check for proper spacing and alignment

### Step 5: Verify Memory Usage

#### Check Flash Memory
```bash
platformio run --environment esp32dev-st7735-debug
# Check "Flash: [====] XX.X% used"
```

#### Memory Guidelines
- Each 8x11 character uses ~11 bytes of flash memory
- ESP32 has sufficient space for 100+ additional characters
- Monitor memory usage if adding large character sets

## Character Set Examples

### French Extended Support
```cpp
// Common French characters
{0x00E9, 8, 11, 0, 2, 8, glyph_e_acute},         // é
{0x00E8, 8, 11, 0, 2, 8, glyph_e_grave},         // è
{0x00E0, 8, 11, 0, 2, 8, glyph_a_grave},         // à
{0x00E7, 8, 11, 0, 2, 8, glyph_c_cedilla},       // ç
{0x00EA, 8, 11, 0, 2, 8, glyph_e_circumflex},    // ê
```

### Spanish Extended Support
```cpp
// Spanish characters
{0x00F1, 8, 11, 0, 2, 8, glyph_n_tilde},         // ñ
{0x00D1, 8, 13, 0, 0, 8, glyph_N_tilde},         // Ñ
{0x00E1, 8, 11, 0, 2, 8, glyph_a_acute},         // á
{0x00ED, 8, 11, 0, 2, 8, glyph_i_acute},         // í
{0x00F3, 8, 11, 0, 2, 8, glyph_o_acute},         // ó
```

### Ukrainian Cyrillic Support
```cpp
// Ukrainian Cyrillic characters
{0x0410, 8, 13, 0, 0, 8, glyph_cyrillic_A},      // А
{0x0411, 8, 13, 0, 0, 8, glyph_cyrillic_BE},     // Б
{0x0413, 8, 13, 0, 0, 8, glyph_cyrillic_GHE},    // Г
{0x0414, 8, 13, 0, 0, 8, glyph_cyrillic_DE},     // Д
{0x043E, 8, 11, 0, 2, 8, glyph_cyrillic_o},      // о
```

## Troubleshooting

### Character Not Displaying
1. **Check Unicode codepoint** - Verify correct Unicode value
2. **Verify bitmap data** - Ensure bitmap array is properly formatted
3. **Check positioning** - Adjust yOffset if character appears misaligned
4. **Rebuild and upload** - Ensure changes are compiled and uploaded

### Character Appears Corrupted
1. **Review bitmap data** - Check for errors in hex values
2. **Verify dimensions** - Ensure width/height match bitmap data
3. **Check byte alignment** - Ensure each row is properly aligned

### Display Issues
1. **Memory overflow** - Check if too many characters added
2. **Flash memory full** - Monitor flash usage percentage
3. **Spacing problems** - Adjust xAdvance value for proper spacing

## Technical Details

### File Locations
- **Header file**: `/include/text_utils.h` - Function declarations
- **Implementation**: `/src/text_utils.cpp` - Unicode rendering logic
- **Integration**: `/src/display_clock_manager.cpp` - Display integration

### Character Flow
1. **User input** → Web interface text field
2. **HTTP POST** → `/clock-label` endpoint
3. **Storage** → TimeManager + LittleFS persistence
4. **Retrieval** → DisplayClockManager::getClockLabel()
5. **Rendering** → TextUtils::drawUnicodeText()

### Technical Architecture
- **UTF-8 decoding** - Converts UTF-8 bytes to Unicode codepoints
- **Glyph lookup** - Finds bitmap data for each character
- **Pixel rendering** - Draws character bitmaps to TFT display
- **Fallback handling** - Uses standard fonts for unsupported characters

## Support

For additional character support or technical assistance:
1. Create GitHub issue with specific character requirements
2. Include target language and character samples
3. Specify use case and priority level

The Unicode system is designed to be easily extensible - adding new characters typically requires only bitmap creation and a few lines of code changes.
