/**
 * @file text_utils.h
 * @brief Unicode text rendering utilities for international character support
 * 
 * This module provides true Unicode character rendering for TFT displays,
 * supporting German umlauts and Ukrainian Cyrillic characters with actual
 * glyph rendering instead of ASCII substitutions.
 * 
 * @author Dual Display Billboard Project
 * @version 2.0
 * @date August 2025
 */

#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * @brief Unicode glyph definition structure
 */
struct UnicodeGlyph {
    uint16_t codepoint;     // Unicode codepoint (e.g., 0x00FC for ü)
    uint8_t width;          // Glyph width in pixels
    uint8_t height;         // Glyph height in pixels
    int8_t xOffset;         // X offset for positioning
    int8_t yOffset;         // Y offset for positioning
    uint8_t xAdvance;       // Horizontal advance after drawing
    const uint8_t* bitmap;  // Glyph bitmap data
};

/**
 * @brief Unicode text rendering utilities
 */
class TextUtils {
public:
    /**
     * @brief Render Unicode text string at specified position
     * @param tft TFT display instance
     * @param text UTF-8 encoded text string
     * @param x X position for text start
     * @param y Y position for text baseline
     * @param color Text color
     * @return Width of rendered text in pixels
     */
    static int drawUnicodeText(TFT_eSPI& tft, const String& text, int x, int y, uint16_t color = TFT_WHITE);
    
    /**
     * @brief Calculate pixel width of Unicode text string
     * @param tft TFT display instance for accurate font measurements
     * @param text UTF-8 encoded text string
     * @return Total width in pixels
     */
    static int getUnicodeTextWidth(TFT_eSPI& tft, const String& text);
    
    /**
     * @brief Convert international characters to display-ready text
     * @param input Input string that may contain international characters
     * @return String ready for display (Unicode rendering or ASCII fallback)
     */
    static String toDisplayText(const String& input);
    
    /**
     * @brief Convert international characters to ASCII fallback text
     * @param input Input string that may contain international characters
     * @return String with ASCII substitutions
     */
    static String transliterateText(const String& input);

private:
    /**
     * @brief Find glyph data for Unicode codepoint
     * @param codepoint Unicode codepoint to find
     * @return Pointer to glyph data or nullptr if not found
     */
    static const UnicodeGlyph* findGlyph(uint16_t codepoint);
    
    /**
     * @brief Get next UTF-8 character from string
     * @param text Text string
     * @param index Current index (will be updated)
     * @return Unicode codepoint
     */
    static uint16_t getNextUTF8Char(const String& text, int& index);
    
    /**
     * @brief Draw individual Unicode glyph
     * @param tft TFT display instance
     * @param glyph Glyph to draw
     * @param x X position
     * @param y Y position
     * @param color Color to use
     */
    static void drawGlyph(TFT_eSPI& tft, const UnicodeGlyph* glyph, int x, int y, uint16_t color);
    
    /**
     * @brief Fallback to ASCII substitution if Unicode glyph not available
     * @param text Input text
     * @return ASCII equivalent text
     */
    static String toASCIIFallback(const String& text);
};

#endif // TEXT_UTILS_H
