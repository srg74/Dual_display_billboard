/**
 * @file clock_types.h
 * @brief Clock face type definitions for dual display billboard system
 * 
 * Defines the available clock face styles for the professional clock display
 * system. Each clock type provides a distinct visual presentation optimized
 * for different aesthetic preferences and display environments.
 * 
 * Clock Face Design Philosophy:
 * - Classic Analog: Traditional circular design with analog hands
 * - Digital Modern: Contemporary digital time display with enhanced readability
 * - Minimalist: Clean, simple design focusing on essential information
 * - Modern Square: Geometric square layout with contemporary styling
 * 
 * All clock faces are designed for optimal readability on both ST7735 (160x80)
 * and ST7789 (240x240) display configurations with automatic scaling and
 * layout adjustments.
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 0.9
 * @since v0.9
 */

#pragma once

/**
 * @brief Available clock face types for display presentation
 * 
 * Enumeration defining the visual styles available for clock display
 * on the dual display billboard system. Each type represents a complete
 * clock rendering implementation with optimized graphics and layout.
 * 
 * Clock face selection affects:
 * - Visual appearance and layout style
 * - Information density and readability
 * - Rendering performance and memory usage
 * - User experience and aesthetic appeal
 */
enum ClockFaceType {
    /**
     * @brief Traditional circular analog clock with hour and minute hands
     * 
     * Classic round clock face featuring traditional analog display with
     * hour markers, minute markers, and rotating hands. Provides familiar
     * time reading experience with elegant visual presentation.
     * 
     * Features:
     * - Circular clock face with 12-hour markers
     * - Hour and minute hands with smooth animation
     * - Traditional Roman or Arabic numerals
     * - Optimal for users preferring analog time display
     */
    CLOCK_CLASSIC_ANALOG = 0,
    
    /**
     * @brief Contemporary digital time display with modern styling
     * 
     * Large digital time presentation with modern typography and clean
     * layout. Emphasizes high readability with bold digit display and
     * contemporary design elements.
     * 
     * Features:
     * - Large, bold digital time display
     * - Date information integration
     * - Modern typography and spacing
     * - High contrast for excellent readability
     */
    CLOCK_DIGITAL_MODERN = 1,
    
    /**
     * @brief Clean, simple design focusing on essential time information
     * 
     * Minimalist approach emphasizing clarity and simplicity. Reduces
     * visual clutter while maintaining excellent readability and
     * professional appearance.
     * 
     * Features:
     * - Clean, uncluttered layout
     * - Essential time information only
     * - Subtle design elements
     * - Optimal for professional environments
     */
    CLOCK_MINIMALIST = 2,
    
    /**
     * @brief Contemporary square layout with geometric styling
     * 
     * Modern geometric design utilizing square layout principles.
     * Combines contemporary aesthetics with functional time display
     * in a distinctive square format.
     * 
     * Features:
     * - Square geometric layout
     * - Contemporary design elements
     * - Balanced information presentation
     * - Unique visual identity
     */
    CLOCK_MODERN_SQUARE = 3
};
