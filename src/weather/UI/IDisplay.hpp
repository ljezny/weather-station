#pragma once
#include "DisplayTypes.hpp"  // Common types: Alignment, Font, Rectangle_t, colors

/**
 * Abstract display interface for e-paper displays.
 * Implementations handle specific display types (7.5" BWR, 10.2" greyscale, etc.)
 */
class IDisplay
{
public:
    virtual ~IDisplay() = default;
    
    // Lifecycle management
    virtual void beginDraw() = 0;
    virtual void endDraw() = 0;
    virtual void clear() = 0;
    
    // Display dimensions
    virtual int getDisplayWidth() = 0;
    virtual int getDisplayHeight() = 0;
    
    // Font scale management (for accessibility)
    static void setFontScale(FontScale_t scale);
    static FontScale_t getFontScale();
    static int getSpacing();
    static int getMargin();
    
    // Text rendering
    virtual Rectangle_t drawText(Font font, String text, int x, int y, 
        Alignment horizontal, Alignment vertical, 
        int margin_horizontal, int margin_vertical, 
        int foregroundColor, int backgroundColor = -1) = 0;
    
    virtual Rectangle_t drawTextScaled(Font font, String text, int x, int y,
        Alignment horizontal, Alignment vertical,
        int margin_horizontal, int margin_vertical,
        int foregroundColor, int backgroundColor = -1) = 0;
    
    virtual Rectangle_t drawTextMultiline(Font font, const char* lines[], int lineCount,
        int x, int y, int width, int height,
        Alignment horizontal, Alignment vertical, int foregroundColor) = 0;
    
    virtual Rectangle_t measureText(Font font, String text, int x, int y,
        Alignment horizontal, Alignment vertical,
        int margin_horizontal, int margin_vertical) = 0;
    
    virtual Rectangle_t measureTextScaled(Font font, String text, int x, int y,
        Alignment horizontal, Alignment vertical,
        int margin_horizontal, int margin_vertical) = 0;
    
    virtual int getLineHeight(Font font) = 0;
    virtual int getLineHeightScaled(Font font) = 0;
    virtual int getTextHeight(Font font, String text) = 0;
    virtual int getTextHeightScaled(Font font, String text) = 0;
    
    // Graphics primitives
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void drawImage(const uint8_t *data, int x, int y, int w, int h,
        Alignment horizontal, Alignment vertical, int margin) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void fillRectRounded(int x, int y, int w, int h, int cornerRadius, int color) = 0;
    virtual void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) = 0;
    virtual void drawCatmullRomCurve(int x[], int y[], int n, int thickness, int color) = 0;
    virtual void invertRect(int x, int y, int w, int h) = 0;
    
    // Display update
    virtual void updateFullscreen() = 0;
    virtual void clearWindow(int x, int y, int w, int h) = 0;
    virtual void updateWindow(int x, int y, int w, int h) = 0;
    
    // Paging support (for displays that require it)
    virtual bool needsPaging() { return false; }
    virtual void firstPage() {}
    virtual bool nextPage() { return false; }
    
    // Power management hints
    virtual bool canLightSleep() = 0;

protected:
    static FontScale_t currentFontScale;
};

// Static member definition - shared across all display implementations
// Implementation is in IDisplay.cpp
