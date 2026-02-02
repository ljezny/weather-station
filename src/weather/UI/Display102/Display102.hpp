#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <driver/gpio.h>
#include "../Adafruit_GFX/Adafruit_GFX.h"
#include "../DisplayTypes.hpp"
#include "../../Logging/Logging.hpp"

// Pin definitions - same as 7.5" display
#define PIN_PWR_EN 2
#define PIN_BUSY 4
#define PIN_RST 16
#define PIN_DC 17
#define PIN_CS 5

// Physical display dimensions for GDEM102T91 (10.2")
#define EPD102_WIDTH 960
#define EPD102_HEIGHT 640
#define EPD102_PIXEL_PER_BYTE 8
#define EPD102_PAGES 2
#define EPD102_ARRAY (EPD102_WIDTH * EPD102_HEIGHT / EPD102_PIXEL_PER_BYTE / EPD102_PAGES)

// Portrait mode - rotate 90 degrees (same as 7.5")
#define PORTRAIT_MODE_102 1

#if PORTRAIT_MODE_102
    #define EPD102_VIRTUAL_WIDTH EPD102_HEIGHT
    #define EPD102_VIRTUAL_HEIGHT EPD102_WIDTH
#else
    #define EPD102_VIRTUAL_WIDTH EPD102_WIDTH
    #define EPD102_VIRTUAL_HEIGHT EPD102_HEIGHT
#endif

// Color definitions for 4-level greyscale
#define GxEPD_BLACK     0x0000
#define GxEPD_VERYDARK  0x39E7  // 64,64,64 - very dark grey for RED mapping
#define GxEPD_DARKGREY  0x7BEF  // 128,128,128
#define GxEPD_LIGHTGREY 0xC618  // 192,192,192
#define GxEPD_WHITE     0xFFFF

/**
 * Display driver for 10.2" GDEM102T91 e-paper display.
 * Features:
 * - 960x640 resolution
 * - 4-level greyscale (with dithering)
 * - Paging (2 pages due to memory constraints)
 * - SPI interface (same pins as 7.5")
 * 
 * Interface matches Display (7.5") for interchangeability via typedef.
 */
class Display102 : public Adafruit_GFX
{
public:
    Display102();
    ~Display102();
    
    // Prevent copying - frameBuffer would be double-freed
    Display102(const Display102&) = delete;
    Display102& operator=(const Display102&) = delete;
    
    // Font scale for accessibility (static, shared with IDisplay)
    static void setFontScale(FontScale_t scale);
    static FontScale_t getFontScale();
    static int getSpacing();
    static int getMargin();
    
    // Lifecycle
    void beginDraw();
    void endDraw();
    void clear();
    
    int getDisplayWidth();
    int getDisplayHeight();
    
    Rectangle_t drawText(Font font, String text, int x, int y,
        Alignment horizontal, Alignment vertical,
        int margin_horizontal, int margin_vertical,
        int foregroundColor, int backgroundColor = -1);
    
    Rectangle_t drawTextScaled(Font font, String text, int x, int y,
        Alignment horizontal, Alignment vertical,
        int margin_horizontal, int margin_vertical,
        int foregroundColor, int backgroundColor = -1);
    
    Rectangle_t drawTextMultiline(Font font, const char* lines[], int lineCount,
        int x, int y, int width, int height,
        Alignment horizontal, Alignment vertical, int foregroundColor);
    
    Rectangle_t measureText(Font font, String text, int x, int y,
        Alignment horizontal, Alignment vertical,
        int margin_horizontal, int margin_vertical);
    
    Rectangle_t measureTextScaled(Font font, String text, int x, int y,
        Alignment horizontal, Alignment vertical,
        int margin_horizontal, int margin_vertical);
    
    int getLineHeight(Font font);
    int getLineHeightScaled(Font font);
    int getTextHeight(Font font, String text);
    int getTextHeightScaled(Font font, String text);
    
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void drawImage(const uint8_t *data, int x, int y, int w, int h,
        Alignment horizontal, Alignment vertical, int margin);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRectRounded(int x, int y, int w, int h, int cornerRadius, int color);
    void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
    void drawCatmullRomCurve(int x[], int y[], int n, int thickness, int color);
    void invertRect(int x, int y, int w, int h);
    
    void updateFullscreen();
    void clearWindow(int x, int y, int w, int h);
    void updateWindow(int x, int y, int w, int h);
    
    // Paging support - required for 10.2" display
    bool needsPaging() { return !_popupMode; }  // No paging in popup mode
    void firstPage();
    bool nextPage();
    
    bool canLightSleep();
    
    // Fast update mode
    void setFastUpdate(bool fast) { _fastUpdate = fast; }
    
    // Popup mode - disables paging, stores only popup window in buffer
    void setPopupMode(bool enabled, int x = 0, int y = 0, int w = 0, int h = 0);

private:
    uint8_t *frameBuffer = nullptr;
    int currentPage = 0;
    bool _fastUpdate = false;  // Default to FULL refresh for proper contrast
    bool _mirror = false;
    bool _popupMode = false;   // Popup mode - no paging, windowed buffer
    int _popupX = 0, _popupY = 0, _popupW = 0, _popupH = 0;  // Popup virtual coords
    static FontScale_t currentFontScale;
    
    // Font helpers
    const GFXfont* getFont(Font font);
    const GFXfont* getScaledFont(Font font);
    
    // Color mapping - maps unified colors to greyscale
    uint16_t mapColor(uint16_t color);
    
    // SPI communication
    void writeSPI(unsigned char value);
    void writeData(unsigned char data);
    void writeCommand(unsigned char command);
    void waitBusy();
    
    // Power management
    void powerOn();
    void powerOff();
    void init();
    
    // Window setting
    void setFullWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    
    // Coordinate transformation helpers
    template <typename T>
    static inline void swapCoords(T &a, T &b) {
        T t = a;
        a = b;
        b = t;
    }
};
