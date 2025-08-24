/**
 * @file text_utils.cpp
 * @brief Unicode text rendering implementation
 * 
 * This module implements Unicode character rendering for TFT displays,
 * providing actual glyph rendering for German umlauts instead of ASCII substitutions.
 * 
 * @author Dual Display Billboard Project
 * @version 2.0
 * @date August 2025
 */

#include "text_utils.h"

// Font size matches TFT_eSPI font 1
static const uint8_t FONT_HEIGHT = 16;
static const uint8_t FONT_WIDTH = 8;
static const uint8_t LOWERCASE_HEIGHT = 11; // Lowercase letters are shorter

// Bitmap data for lowercase ü (8x11 pixels, cleaner design)
static const uint8_t glyph_u_umlaut_small[] = {
    0x00,  // ........
    0x00,  // ........
    0x66,  // .##..##.  (umlaut dots, better positioned)
    0x00,  // ........
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x46,  // .#...##.
    0x3A   // ..###.#.
};

// Bitmap data for lowercase ä (8x11 pixels, cleaner design)
static const uint8_t glyph_a_umlaut_small[] = {
    0x00,  // ........
    0x00,  // ........
    0x66,  // .##..##.  (umlaut dots, better positioned)
    0x00,  // ........
    0x3C,  // ..####..
    0x02,  // ......#.
    0x3E,  // ..#####.
    0x42,  // .#....#.
    0x46,  // .#...##.
    0x3A,  // ..###.#.
    0x00   // ........
};

// Bitmap data for lowercase ö (8x11 pixels, cleaner design)
static const uint8_t glyph_o_umlaut_small[] = {
    0x00,  // ........
    0x00,  // ........
    0x66,  // .##..##.  (umlaut dots, better positioned)
    0x00,  // ........
    0x3C,  // ..####..
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x3C,  // ..####..
    0x00   // ........
};

// Bitmap data for uppercase Ü (8x16 pixels)
static const uint8_t glyph_u_umlaut[] = {
    0x00,  // ........
    0x24,  // ..#..#..
    0x00,  // ........
    0x00,  // ........
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x3C,  // ..####..
    0x00,  // ........
    0x00   // ........
};

// Bitmap data for uppercase Ä (8x16 pixels)
static const uint8_t glyph_a_umlaut[] = {
    0x00,  // ........
    0x00,  // ........
    0x24,  // ..#..#..
    0x24,  // ..#..#..
    0x00,  // ........
    0x3C,  // ..####..
    0x42,  // .#....#.
    0x02,  // ......#.
    0x3E,  // ..#####.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x46,  // .#...##.
    0x3A,  // ..###.#.
    0x00,  // ........
    0x00,  // ........
    0x00   // ........
};

// Bitmap data for uppercase Ö (8x16 pixels)
static const uint8_t glyph_o_umlaut[] = {
    0x00,  // ........
    0x00,  // ........
    0x24,  // ..#..#..
    0x24,  // ..#..#..
    0x00,  // ........
    0x3C,  // ..####..
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x42,  // .#....#.
    0x3C,  // ..####..
    0x00,  // ........
    0x00,  // ........
    0x00   // ........
};

// Glyph table for Unicode characters (German only for now)
static const UnicodeGlyph unicode_glyphs[] = {
    // German umlauts (lowercase use smaller bitmaps with proper baseline alignment)
    {0x00FC, 8, 11, 0, 2, 8, glyph_u_umlaut_small},   // ü (lowercase, 2px offset to align baseline)
    {0x00E4, 8, 11, 0, 2, 8, glyph_a_umlaut_small},   // ä (lowercase, 2px offset to align baseline)
    {0x00F6, 8, 11, 0, 2, 8, glyph_o_umlaut_small},   // ö (lowercase, 2px offset to align baseline)
    {0x00DC, 8, 16, 0, 0, 8, glyph_u_umlaut},   // Ü (uppercase, full height)
    {0x00C4, 8, 16, 0, 0, 8, glyph_a_umlaut},   // Ä (uppercase, full height)
    {0x00D6, 8, 16, 0, 0, 8, glyph_o_umlaut},   // Ö (uppercase, full height)
};

static const int GLYPH_COUNT = sizeof(unicode_glyphs) / sizeof(UnicodeGlyph);

/**
 * @brief Render Unicode text string at specified position
 */
/**
 * @brief Draw Unicode text string on TFT display with platform-specific adjustments
 */
int TextUtils::drawUnicodeText(TFT_eSPI& tft, const String& text, int x, int y, uint16_t color) {
    // Platform-specific positioning adjustments for ESP32S3
    // ESP32S3 has different SPI timing and memory alignment affecting text positioning
#ifdef ESP32S3_MODE
    // ESP32S3: Adjust Y position for better umlaut alignment
    y += 1;  // Slight downward shift to compensate for memory alignment differences
    // ESP32S3: Adjust X position for better character spacing
    x += 1;  // Slight rightward shift for better centering
#endif

    int currentX = x;
    int index = 0;
    
    while (index < text.length()) {
        int oldIndex = index; // Save index in case we need to detect infinite loops
        uint16_t codepoint = getNextUTF8Char(text, index);
        
        // Prevent infinite loops - if index didn't advance, force it
        if (index == oldIndex) {
            index++;
        }
        
        // If we get 0, only break if we're actually at the end
        if (codepoint == 0 && index >= text.length()) {
            break;
        }
        
        // Try to find Unicode glyph
        const UnicodeGlyph* glyph = findGlyph(codepoint);
        if (glyph) {
            drawGlyph(tft, glyph, currentX, y, color);
            int advance = glyph->xAdvance;
            currentX += advance;
        } else if (codepoint > 0 && codepoint < 128) {
            // ASCII character - use built-in font
            tft.setTextFont(2);
            tft.setTextColor(color);
            tft.drawChar(codepoint, currentX, y);
            int advance = tft.textWidth(String((char)codepoint));
            currentX += advance;
        } else if (codepoint > 0) {
            // Unknown Unicode character - draw placeholder or skip
            int advance = FONT_WIDTH;
            currentX += advance;
        }
    }
    
    return currentX - x; // Return total width
}

/**
 * @brief Calculate pixel width of Unicode text string
 */
int TextUtils::getUnicodeTextWidth(TFT_eSPI& tft, const String& text) {
    int totalWidth = 0;
    int index = 0;
    
    // Set font to match what we use in rendering
    tft.setTextFont(2);
    
    while (index < text.length()) {
        uint16_t codepoint = getNextUTF8Char(text, index);
        
        // If we get 0, it means we hit an invalid sequence or end of string
        if (codepoint == 0) {
            break;
        }
        
        const UnicodeGlyph* glyph = findGlyph(codepoint);
        if (glyph) {
            totalWidth += glyph->xAdvance;
        } else if (codepoint < 128) {
            // ASCII character - use actual TFT font width measurement
            totalWidth += tft.textWidth(String((char)codepoint));
        } else {
            totalWidth += 6; // Default width for unknown (font 1 standard)
        }
    }
    
    return totalWidth;
}

/**
 * @brief Convert international characters to display-ready text
 */
String TextUtils::transliterateText(const String& input) {
    String result = "";
    int index = 0;
    
    while (index < input.length()) {
        uint16_t codepoint = getNextUTF8Char(input, index);
        
        // If we get 0, we've hit the end or an invalid sequence
        if (codepoint == 0) {
            break;
        }
        
        // Check if we have a Unicode glyph for this character
        const UnicodeGlyph* glyph = findGlyph(codepoint);
        if (glyph) {
            // We can render this Unicode character directly
            result += String((char*)&codepoint); // This might not work correctly for Unicode
        } else if (codepoint < 128) {
            // ASCII character
            result += (char)codepoint;
        } else {
            // Character mapping for display (fallback for characters we can't render)
            switch (codepoint) {
                case 0x00FC: result += "ue"; break; // ü
                case 0x00E4: result += "ae"; break; // ä
                case 0x00F6: result += "oe"; break; // ö
                case 0x00DC: result += "UE"; break; // Ü
                case 0x00C4: result += "AE"; break; // Ä
                case 0x00D6: result += "OE"; break; // Ö
                case 0x00DF: result += "ss"; break; // ß
                default:
                    result += "?"; // Unknown character
                    break;
            }
        }
    }
    
    return result;
}

/**
 * @brief Alias for transliterateText - for compatibility
 */
String TextUtils::toDisplayText(const String& input) {
    return input; // Return the original string for Unicode rendering
}

/**
 * @brief Find glyph for Unicode codepoint
 */
const UnicodeGlyph* TextUtils::findGlyph(uint16_t codepoint) {
    for (int i = 0; i < GLYPH_COUNT; i++) {
        if (unicode_glyphs[i].codepoint == codepoint) {
            return &unicode_glyphs[i];
        }
    }
    return nullptr;
}

/**
 * @brief Get next UTF-8 character from string
 */
uint16_t TextUtils::getNextUTF8Char(const String& text, int& index) {
    if (index >= text.length()) {
        return 0;
    }
    
    uint8_t firstByte = text[index++];
    
    // ASCII character (0xxxxxxx)
    if ((firstByte & 0x80) == 0) {
        return firstByte;
    }
    
    // 2-byte UTF-8 (110xxxxx 10xxxxxx)
    if ((firstByte & 0xE0) == 0xC0) {
        if (index >= text.length()) {
            index--; // Back up if we don't have enough bytes
            return 0;
        }
        uint8_t secondByte = text[index++];
        // Validate second byte format
        if ((secondByte & 0xC0) != 0x80) {
            index--; // Back up on invalid sequence
            return firstByte; // Return first byte as fallback
        }
        return ((firstByte & 0x1F) << 6) | (secondByte & 0x3F);
    }
    
    // 3-byte UTF-8 (1110xxxx 10xxxxxx 10xxxxxx)
    if ((firstByte & 0xF0) == 0xE0) {
        if (index + 1 >= text.length()) {
            index--; // Back up if we don't have enough bytes
            return 0;
        }
        uint8_t secondByte = text[index++];
        uint8_t thirdByte = text[index++];
        // Validate byte formats
        if ((secondByte & 0xC0) != 0x80 || (thirdByte & 0xC0) != 0x80) {
            index -= 2; // Back up on invalid sequence
            return firstByte; // Return first byte as fallback
        }
        return ((firstByte & 0x0F) << 12) | ((secondByte & 0x3F) << 6) | (thirdByte & 0x3F);
    }
    
    // Invalid or unsupported UTF-8 - return as single byte
    return firstByte;
}

/**
 * @brief Draw individual Unicode glyph
 */
void TextUtils::drawGlyph(TFT_eSPI& tft, const UnicodeGlyph* glyph, int x, int y, uint16_t color) {
    if (!glyph || !glyph->bitmap) return;
    
    for (int row = 0; row < glyph->height; row++) {
        uint8_t rowData = glyph->bitmap[row];
        
        for (int col = 0; col < glyph->width; col++) {
            // Check if pixel should be drawn (MSB first)
            if (rowData & (0x80 >> col)) {
                tft.drawPixel(x + glyph->xOffset + col, y + glyph->yOffset + row, color);
            }
        }
    }
}
