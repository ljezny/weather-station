#include "Display102.hpp"
#include "../DisplayTypes.hpp"  // For COLOR_RED
#include <WiFi.h>
#include "../gfxlatin2.h"

// Include fonts (same as 7.5" display)
#include "../InterTightSemiBold6pt8bfr.h"
#include "../InterTightBold8pt8bfr.h"
#include "../InterTightSemiBold8pt8bfr.h"
#include "../InterTightSemiBold10pt8bfr.h"
#include "../InterTightSemiBold12pt8bfr.h"
#include "../InterTightSemiBold14pt8bfr.h"
#include "../InterTightBold16pt8bfr.h"
#include "../InterTightBold18pt8bfr.h"
#include "../InterTightBold22pt8bfr.h"
#include "../InterTightBold24pt8bfr.h"
#include "../InterTightBold32pt8bfr.h"
#include "../InterTightBold34pt8bfr.h"
#include "../InterTightBold52pt8bfr.h"

// Static member initialization
FontScale_t Display102::currentFontScale = FONT_SCALE_NORMAL;

void Display102::setFontScale(FontScale_t scale)
{
    currentFontScale = scale;
    Serial.println("[Display102] Font scale set to: " + String((int)scale));
}

FontScale_t Display102::getFontScale()
{
    return currentFontScale;
}

int Display102::getSpacing()
{
    switch (currentFontScale) {
        case FONT_SCALE_SMALL: return 6;
        case FONT_SCALE_LARGE: return 12;
        default: return 8;
    }
}

int Display102::getMargin()
{
    switch (currentFontScale) {
        case FONT_SCALE_SMALL: return 3;
        case FONT_SCALE_LARGE: return 6;
        default: return 4;
    }
}

Display102::Display102() : Adafruit_GFX(EPD102_HEIGHT, EPD102_WIDTH)
{
    // Portrait mode: user sees 640x960, physical display is 960x640
}

Display102::~Display102()
{
    if (frameBuffer != nullptr) {
        free(frameBuffer);
        frameBuffer = nullptr;
    }
}

// ============================================================================
// Color Mapping
// ============================================================================

uint16_t Display102::mapColor(uint16_t color)
{
    // Map unified colors to 4-level greyscale
    if (color == COLOR_BLACK) {
        return GxEPD_BLACK;
    } else if (color == COLOR_RED) {
        // Map RED to solid black (no dithering) for testing
        return GxEPD_BLACK;
    } else if (color == COLOR_WHITE) {
        return GxEPD_WHITE;
    }
    // For any other color, try to map based on brightness
    // Extract approximate brightness from RGB565
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    uint16_t brightness = (r * 8 + g * 4 + b * 8) / 3;  // Approximate 0-255
    
    if (brightness < 64) return GxEPD_BLACK;
    if (brightness < 128) return GxEPD_DARKGREY;
    if (brightness < 192) return GxEPD_LIGHTGREY;
    return GxEPD_WHITE;
}

// ============================================================================
// Lifecycle Management
// ============================================================================

void Display102::beginDraw()
{
    LOGD("Display102::beginDraw - Allocating frame buffer, max alloc: " + String(ESP.getMaxAllocHeap()));
    
    if (frameBuffer == nullptr) {
        frameBuffer = (uint8_t *)malloc(EPD102_ARRAY);
        if (frameBuffer == nullptr) {
            LOGD("Display102: Failed to allocate frame buffer!");
            return;
        }
    }
    memset(frameBuffer, 0xFF, EPD102_ARRAY);
    LOGD("Display102::beginDraw - Buffer allocated, max alloc now: " + String(ESP.getMaxAllocHeap()));
    
    // Power on and initialize display
    powerOn();
    init();
}

void Display102::endDraw()
{
    LOGD("Display102::endDraw - Freeing frame buffer");
    
    // Power off display
    powerOff();
    
    if (frameBuffer != nullptr) {
        free(frameBuffer);
        frameBuffer = nullptr;
    }
    LOGD("Display102::endDraw - Buffer freed, max alloc now: " + String(ESP.getMaxAllocHeap()));
}

void Display102::clear()
{
    if (frameBuffer != nullptr) {
        memset(frameBuffer, 0xFF, EPD102_ARRAY);
    }
}

// ============================================================================
// Display Dimensions
// ============================================================================

int Display102::getDisplayWidth()
{
    return EPD102_HEIGHT;  // 640 (portrait width)
}

int Display102::getDisplayHeight()
{
    return EPD102_WIDTH;  // 960 (portrait height)
}

// ============================================================================
// Power Management
// ============================================================================

void Display102::powerOn()
{
    pinMode(PIN_PWR_EN, OUTPUT);
    pinMode(PIN_BUSY, INPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_CS, OUTPUT);
    
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    SPI.begin();
    
    digitalWrite(PIN_PWR_EN, HIGH);
    delay(100);
    LOGD("Display102: Power ON");
}

void Display102::powerOff()
{
    writeCommand(0x10);  // Enter deep sleep
    writeData(0x01);
    delay(100);
    
    SPI.end();
    SPI.endTransaction();
    
    digitalWrite(PIN_PWR_EN, LOW);
    LOGD("Display102: Power OFF");
}

void Display102::init()
{
    // Hardware reset
    digitalWrite(PIN_RST, LOW);
    delay(10);  // At least 10ms delay
    digitalWrite(PIN_RST, HIGH);
    delay(10);  // At least 10ms delay
    waitBusy();
    LOGD("Display102: Reset complete");
    
    writeCommand(0x12);  // SWRESET
    waitBusy();          // Wait for BUSY after SW reset
    LOGD("Display102: SW reset complete");
    
    writeCommand(0x0C);  // Soft start setting
    writeData(0xAE);
    writeData(0xC7);
    writeData(0xC3);
    writeData(0xC0);
    writeData(0xFF);     // 0x80 to 0xFF
    
    writeCommand(0x01);  // Set MUX as 639 (0x27F)
    writeData(0x7F);     // Low byte
    writeData(0x02);     // High byte
    writeData(0x00);
    
    setFullWindow(0, 0, EPD102_WIDTH, EPD102_HEIGHT);
    
    writeCommand(0x3C);  // VBD (Border Waveform)
    writeData(0x01);     // LUT1, for white
    
    writeCommand(0x18);  // Temperature sensor
    writeData(0x80);     // Internal temperature sensor
    
    // Set RAM address counters to start position
    writeCommand(0x4E);
    writeData(0x00);
    writeData(0x00);
    writeCommand(0x4F);
    writeData(0x00);
    writeData(0x00);
    
    LOGD("Display102: Init complete");
}

void Display102::setFullWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    writeCommand(0x11);  // Set ram entry mode
    writeData(0x03);     // x increase, y increase : normal mode
    
    // RAM x address range
    writeCommand(0x44);
    writeData(x % 256);
    writeData(x / 256);
    writeData((x + w - 1) % 256);
    writeData((x + w - 1) / 256);
    
    // RAM y address range
    writeCommand(0x45);
    writeData(y % 256);
    writeData(y / 256);
    writeData((y + h - 1) % 256);
    writeData((y + h - 1) / 256);
    
    // Set RAM counters to start position
    writeCommand(0x4E);
    writeData(x % 256);
    writeData(x / 256);
    writeCommand(0x4F);
    writeData(y % 256);
    writeData(y / 256);
}

// ============================================================================
// SPI Communication
// ============================================================================

void Display102::writeSPI(unsigned char value)
{
    SPI.transfer(value);
}

void Display102::writeData(unsigned char data)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    writeSPI(data);
    digitalWrite(PIN_CS, HIGH);
}

void Display102::writeCommand(unsigned char command)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, LOW);
    writeSPI(command);
    digitalWrite(PIN_CS, HIGH);
}

void Display102::waitBusy()
{
    // Note: GDEM102T91 uses HIGH = busy (opposite of 7.5")
    const unsigned long timeout = 30000; // 30 second timeout
    unsigned long start = millis();
    
    if (canLightSleep()) {
        while (digitalRead(PIN_BUSY) == 1) {
            if (millis() - start > timeout) {
                LOGD("Display102::waitBusy - TIMEOUT after 30s (light sleep mode)");
                return;
            }
            LOGD("Display102: Entering light sleep");
            gpio_wakeup_enable((gpio_num_t)PIN_BUSY, GPIO_INTR_LOW_LEVEL);
            esp_sleep_enable_gpio_wakeup();
            esp_light_sleep_start();
            gpio_wakeup_disable((gpio_num_t)PIN_BUSY);
            LOGD("Display102: Waking up from light sleep");
            if (digitalRead(PIN_BUSY) == 1) {
                delay(50);
            }
        }
    } else {
        LOGD("Display102::waitBusy - Waiting for BUSY pin (WiFi active)...");
        while (digitalRead(PIN_BUSY) == 1) {
            if (millis() - start > timeout) {
                LOGD("Display102::waitBusy - TIMEOUT after 30s! BUSY pin stuck HIGH");
                return;
            }
            delay(100); // Check every 100ms instead of tight loop
        }
        LOGD("Display102::waitBusy - BUSY released after " + String(millis() - start) + "ms");
    }
}

bool Display102::canLightSleep()
{
    return WiFi.getMode() == WIFI_OFF;
}

// ============================================================================
// Paging
// ============================================================================

void Display102::firstPage()
{
    currentPage = 0;
    
    if (frameBuffer != nullptr) {
        memset(frameBuffer, 0xFF, EPD102_ARRAY);
    }
}

bool Display102::nextPage()
{
    if (frameBuffer == nullptr) {
        LOGD("Display102::nextPage - Frame buffer is NULL");
        return false;
    }
    
    LOGD("Display102::nextPage - Page " + String(currentPage));
    
    bool isFirstPage = (currentPage == 0);
    bool isLastPage = (currentPage == EPD102_PAGES - 1);
    
    if (_fastUpdate) {
        // Partial/fast update mode
        if (isFirstPage) {
            writeCommand(0x3C);  // BorderWaveform
            writeData(0x80);     // 0x80 for partial refresh
        }
        
        // Write data to RAM 0x24
        writeCommand(0x24);
        for (int i = 0; i < EPD102_ARRAY; i++) {
            writeData(frameBuffer[i]);
        }
        
        // Write inverted data to RAM 0x26 (for partial refresh)
        writeCommand(0x26);
        for (int i = 0; i < EPD102_ARRAY; i++) {
            writeData(~frameBuffer[i]);
        }
        
        memset(frameBuffer, 0xFF, EPD102_ARRAY);
        
        if (isLastPage) {
            writeCommand(0x22);  // Display Update Control
            writeData(0xFF);     // Partial update mode
            writeCommand(0x20);  // Activate Display Update Sequence
            waitBusy();
        }
    } else {
        // Full refresh mode - simple: just write to RAM 0x24
        writeCommand(0x24);
        for (int i = 0; i < EPD102_ARRAY; i++) {
            writeData(frameBuffer[i]);
        }
        
        memset(frameBuffer, 0xFF, EPD102_ARRAY);
        
        if (isLastPage) {
            writeCommand(0x22);  // Display Update Control
            writeData(0xF7);     // Full update mode with LUT
            writeCommand(0x20);  // Activate Display Update Sequence
            waitBusy();
        }
    }
    
    currentPage++;
    return currentPage < EPD102_PAGES;
}

// ============================================================================
// Popup Mode
// ============================================================================

void Display102::setPopupMode(bool enabled, int x, int y, int w, int h)
{
    _popupMode = enabled;
    if (enabled) {
        _popupX = x;
        _popupY = y;
        _popupW = w;
        _popupH = h;
        LOGD("Display102::setPopupMode - Enabled, window: x=" + String(x) + " y=" + String(y) + 
             " w=" + String(w) + " h=" + String(h));
    } else {
        LOGD("Display102::setPopupMode - Disabled");
    }
}

// ============================================================================
// Pixel Drawing with Dithering
// ============================================================================

void Display102::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (frameBuffer == nullptr) return;
    if (x < 0 || x >= width() || y < 0 || y >= height()) return;
    
    // Map color with dithering
    // Note: COLOR_RED (0xF800) needs special handling as it's > LIGHTGREY numerically
    bool isOddColumn = x % 2;
    bool isOddLine = y % 2;
    
    if (color == COLOR_RED) {
        // Red: 75% black pattern (3 black, 1 white)
        color = (isOddColumn && isOddLine) ? 1 : 0;
    } else if (color < GxEPD_VERYDARK) {
        color = 0;  // Black
    } else if (color < GxEPD_DARKGREY) {
        // Very dark grey: 3 black pixels, 1 white = 75% black
        color = (isOddColumn && isOddLine) ? 1 : 0;
    } else if (color < GxEPD_LIGHTGREY) {
        color = (isOddColumn != isOddLine) ? 1 : 0;  // Dark grey checkerboard (50%)
    } else if (color < GxEPD_WHITE) {
        color = (isOddColumn || isOddLine) ? 1 : 0;  // Light grey (25% black)
    } else {
        color = 1;  // White
    }
    
    // Rotate 90° CCW for portrait mode:
    // Portrait (x,y) -> Physical (y, HEIGHT-1-x)
    // where HEIGHT = 640 (EPD102_HEIGHT)
    int16_t physX = y;
    int16_t physY = EPD102_HEIGHT - 1 - x;
    
    uint32_t i;
    
    if (_popupMode) {
        // In popup mode, buffer stores only popup window data
        // Convert physical coords to popup-relative coords
        // Popup physical area:
        // phys_x_start = _popupY (aligned to 8)
        // phys_y_start = EPD102_HEIGHT - _popupX - _popupW
        int phys_popup_x_start = (_popupY / 8) * 8;
        int phys_popup_y_start = EPD102_HEIGHT - _popupX - _popupW;
        int phys_popup_x_end = ((_popupY + _popupH + 7) / 8) * 8 - 1;
        int phys_popup_bytes_per_row = (phys_popup_x_end - phys_popup_x_start + 1) / 8;
        
        // Check if this pixel is within popup window
        if (physX < phys_popup_x_start || physX > phys_popup_x_end ||
            physY < phys_popup_y_start || physY >= phys_popup_y_start + _popupW) {
            return;  // Outside popup window
        }
        
        // Calculate buffer index relative to popup window
        int rel_x_byte = (physX - phys_popup_x_start) / 8;
        int rel_y = physY - phys_popup_y_start;
        i = rel_x_byte + rel_y * phys_popup_bytes_per_row;
        
        // Adjust bit position within byte
        int bit_in_byte = 7 - ((physX - phys_popup_x_start) % 8);
        
        if (i >= EPD102_ARRAY) return;  // Safety check
        
        if (color) {
            frameBuffer[i] |= (1 << bit_in_byte);
        } else {
            frameBuffer[i] &= ~(1 << bit_in_byte);
        }
        return;
    }
    
    // Normal paging mode
    // Calculate buffer position using physical coordinates
    i = (physX + physY * EPD102_WIDTH) / EPD102_PIXEL_PER_BYTE;
    i -= currentPage * EPD102_ARRAY;
    
    // Check if within current page
    if (i >= EPD102_ARRAY) return;
    
    // Set or clear bit
    if (color) {
        frameBuffer[i] |= (1 << (7 - physX % 8));
    } else {
        frameBuffer[i] &= ~(1 << (7 - physX % 8));
    }
}

// ============================================================================
// Graphics Primitives
// ============================================================================

void Display102::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    for (int16_t py = y; py < y + h; py++) {
        for (int16_t px = x; px < x + w; px++) {
            drawPixel(px, py, color);
        }
    }
}

void Display102::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    Adafruit_GFX::fillCircle(x0, y0, r, mapColor(color));
}

void Display102::fillRectRounded(int x, int y, int w, int h, int cornerRadius, int color)
{
    if (cornerRadius == 0) {
        fillRect(x, y, w, h, color);
        return;
    }
    fillRect(x, y + cornerRadius, w + 1, h - 2 * cornerRadius, color);
    fillRect(x + cornerRadius, y, w - 2 * cornerRadius + 2, h + 1, color);
    fillCircle(x + cornerRadius, y + cornerRadius, cornerRadius, color);
    fillCircle(x + w - cornerRadius, y + cornerRadius, cornerRadius, color);
    fillCircle(x + cornerRadius, y + h - cornerRadius, cornerRadius, color);
    fillCircle(x + w - cornerRadius, y + h - cornerRadius, cornerRadius, color);
}

void Display102::invertRect(int x, int y, int w, int h)
{
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            if (px < 0 || px >= width() || py < 0 || py >= height()) continue;
            
            // Same rotation as drawPixel: 90° CCW
            int16_t physX = py;
            int16_t physY = EPD102_HEIGHT - 1 - px;
            
            uint32_t i = (physX + physY * EPD102_WIDTH) / EPD102_PIXEL_PER_BYTE;
            i -= currentPage * EPD102_ARRAY;
            
            if (i < EPD102_ARRAY) {
                int bit = 7 - physX % 8;
                frameBuffer[i] ^= (1 << bit);
            }
        }
    }
}

void Display102::drawCatmullRomCurve(int x[], int y[], int n, int thickness, int color)
{
    if (n < 2) return;
    
    float step = 0.01f;
    
    for (int i = 0; i < n - 1; i++) {
        int p0x = (i == 0) ? x[i] : x[i - 1];
        int p0y = (i == 0) ? y[i] : y[i - 1];
        int p1x = x[i], p1y = y[i];
        int p2x = x[i + 1], p2y = y[i + 1];
        int p3x = (i + 2 < n) ? x[i + 2] : x[i + 1];
        int p3y = (i + 2 < n) ? y[i + 2] : y[i + 1];
        
        for (float t = 0; t <= 1.0f; t += step) {
            float t2 = t * t;
            float t3 = t2 * t;
            float x_pos = 0.5f * ((2 * p1x) +
                                  (-p0x + p2x) * t +
                                  (2 * p0x - 5 * p1x + 4 * p2x - p3x) * t2 +
                                  (-p0x + 3 * p1x - 3 * p2x + p3x) * t3);
            float y_pos = 0.5f * ((2 * p1y) +
                                  (-p0y + p2y) * t +
                                  (2 * p0y - 5 * p1y + 4 * p2y - p3y) * t2 +
                                  (-p0y + 3 * p1y - 3 * p2y + p3y) * t3);
            
            fillCircle((int)(x_pos + 0.5f), (int)(y_pos + 0.5f), thickness / 2, color);
        }
    }
}

void Display102::drawImage(const uint8_t *data, int x, int y, int w, int h,
    Alignment horizontal, Alignment vertical, int margin)
{
    switch (horizontal) {
        case TRAILING: x = x - w - margin; break;
        case CENTER: x = x - w / 2; break;
        case LEADING: x = x + margin; break;
    }
    switch (vertical) {
        case TRAILING: y = y - h - margin; break;
        case CENTER: y = y - h / 2; break;
        case LEADING: y = y + margin; break;
    }
    
    // Draw 4-bit greyscale image with dithering
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            uint8_t pixelData = pgm_read_byte(&data[j * (w / 2) + i / 2]);
            uint8_t grey = (i % 2 == 0) ? (pixelData & 0x0F) << 4 : (pixelData & 0xF0);
            grey |= grey >> 4;  // Expand to 8-bit
            
            uint16_t color;
            if (grey < 64) color = GxEPD_BLACK;
            else if (grey < 128) color = GxEPD_DARKGREY;
            else if (grey < 192) color = GxEPD_LIGHTGREY;
            else color = GxEPD_WHITE;
            
            drawPixel(x + i, y + j, color);
        }
    }
}

// ============================================================================
// Display Update
// ============================================================================

void Display102::updateFullscreen()
{
    powerOn();
    init();
    
    firstPage();
    do {
        // Note: The actual drawing should be done by the caller in the paging loop
        // This method just handles the display update sequence
    } while (nextPage());
    
    powerOff();
}

void Display102::clearWindow(int x, int y, int w, int h)
{
    LOGD("Display102::clearWindow not implemented for paging display");
    // Partial update not straightforward with paging - do full update
    fillRect(x, y, w, h, COLOR_WHITE);
}

void Display102::updateWindow(int x, int y, int w, int h)
{
    LOGD("Display102::updateWindow - partial refresh window x=" + String(x) + " y=" + String(y) + " w=" + String(w) + " h=" + String(h));
    
    if (frameBuffer == nullptr) {
        LOGD("Display102::updateWindow - No frame buffer!");
        return;
    }
    
    // Convert virtual (portrait) coordinates to physical (landscape) coordinates
    // Portrait rotation: physX = y, physY = HEIGHT-1-x
    // Window: phys_x = y, phys_y = EPD102_HEIGHT - x - w, phys_w = h, phys_h = w
    int phys_x = y;
    int phys_y = EPD102_HEIGHT - x - w;
    int phys_w = h;  // width becomes height
    int phys_h = w;  // height becomes width
    
    // Align X to byte boundary (8 pixels per byte)
    int phys_x_start = (phys_x / 8) * 8;
    int phys_x_end = ((phys_x + phys_w + 7) / 8) * 8 - 1;
    int phys_y_start = phys_y;
    int phys_y_end = phys_y + phys_h - 1;
    
    // Clamp to display bounds
    if (phys_y_start < 0) phys_y_start = 0;
    if (phys_y_end >= EPD102_HEIGHT) phys_y_end = EPD102_HEIGHT - 1;
    if (phys_x_end >= EPD102_WIDTH) phys_x_end = EPD102_WIDTH - 1;
    
    int bytes_per_row = (phys_x_end - phys_x_start + 1) / 8;
    int window_rows = phys_y_end - phys_y_start + 1;
    
    LOGD("Display102::updateWindow - phys window: x=" + String(phys_x_start) + "-" + String(phys_x_end) + 
         " y=" + String(phys_y_start) + "-" + String(phys_y_end) + 
         " (" + String(bytes_per_row) + " bytes/row x " + String(window_rows) + " rows)");
    
    // Hardware reset before partial refresh
    digitalWrite(PIN_RST, LOW);
    delay(10);
    digitalWrite(PIN_RST, HIGH);
    delay(10);
    waitBusy();
    
    // BorderWaveform for partial refresh
    writeCommand(0x3C);
    writeData(0x80);
    
    // Set RAM window to popup area only
    writeCommand(0x44);  // Set RAM X address
    writeData(phys_x_start % 256);
    writeData(phys_x_start / 256);
    writeData(phys_x_end % 256);
    writeData(phys_x_end / 256);
    
    writeCommand(0x45);  // Set RAM Y address
    writeData(phys_y_start % 256);
    writeData(phys_y_start / 256);
    writeData(phys_y_end % 256);
    writeData(phys_y_end / 256);
    
    // Set RAM counters to window start
    writeCommand(0x4E);
    writeData(phys_x_start % 256);
    writeData(phys_x_start / 256);
    writeCommand(0x4F);
    writeData(phys_y_start % 256);
    writeData(phys_y_start / 256);
    
    LOGD("Display102::updateWindow - Writing to RAM 0x24, popupMode=" + String(_popupMode));
    
    // Write to RAM 0x24 (new data)
    writeCommand(0x24);
    
    if (_popupMode) {
        // In popup mode, buffer is organized as:
        // [row0_byte0, row0_byte1, ..., row1_byte0, ...]
        // where each row has bytes_per_row bytes
        for (int i = 0; i < bytes_per_row * window_rows; i++) {
            if (i < EPD102_ARRAY) {
                writeData(frameBuffer[i]);
            } else {
                writeData(0xFF);
            }
        }
    } else {
        // Normal paging mode - read from current page buffer
        int pageYStart = currentPage * (EPD102_HEIGHT / EPD102_PAGES);
        for (int row = phys_y_start; row <= phys_y_end; row++) {
            for (int col_byte = phys_x_start / 8; col_byte <= phys_x_end / 8; col_byte++) {
                int bufferIndex = col_byte + (row - pageYStart) * (EPD102_WIDTH / 8);
                if (row >= pageYStart && row < pageYStart + (EPD102_HEIGHT / EPD102_PAGES) && 
                    bufferIndex >= 0 && bufferIndex < EPD102_ARRAY) {
                    writeData(frameBuffer[bufferIndex]);
                } else {
                    writeData(0xFF);
                }
            }
        }
    }
    
    // Reset RAM counters for 0x26
    writeCommand(0x4E);
    writeData(phys_x_start % 256);
    writeData(phys_x_start / 256);
    writeCommand(0x4F);
    writeData(phys_y_start % 256);
    writeData(phys_y_start / 256);
    
    LOGD("Display102::updateWindow - Writing to RAM 0x26");
    writeCommand(0x26);
    
    if (_popupMode) {
        for (int i = 0; i < bytes_per_row * window_rows; i++) {
            if (i < EPD102_ARRAY) {
                writeData(~frameBuffer[i]);
            } else {
                writeData(0x00);
            }
        }
    } else {
        int pageYStart = currentPage * (EPD102_HEIGHT / EPD102_PAGES);
        for (int row = phys_y_start; row <= phys_y_end; row++) {
            for (int col_byte = phys_x_start / 8; col_byte <= phys_x_end / 8; col_byte++) {
                int bufferIndex = col_byte + (row - pageYStart) * (EPD102_WIDTH / 8);
                if (row >= pageYStart && row < pageYStart + (EPD102_HEIGHT / EPD102_PAGES) && 
                    bufferIndex >= 0 && bufferIndex < EPD102_ARRAY) {
                    writeData(~frameBuffer[bufferIndex]);
                } else {
                    writeData(0x00);
                }
            }
        }
    }
    
    LOGD("Display102::updateWindow - Triggering partial update 0xFF");
    writeCommand(0x22);
    writeData(0xFF);
    writeCommand(0x20);
    waitBusy();
    LOGD("Display102::updateWindow - Partial refresh complete");
}

// ============================================================================
// Font Management
// ============================================================================

const GFXfont* Display102::getScaledFont(Font font)
{
    FontScale_t scale = getFontScale();
    
    if (scale == FONT_SCALE_SMALL) {
        switch (font) {
            case EXTRA_SMALL: return &InterTight_SemiBold6pt8b;
            case TINY: return &InterTight_Bold8pt8b;
            case SMALL: return &InterTight_SemiBold10pt8b;
            case DAY_NUMBER: return &InterTight_SemiBold12pt8b;
            case MEDIUM: return &InterTight_SemiBold14pt8b;
            case LARGE: return &InterTight_Bold18pt8b;
            case EXTRA_LARGE:
            case HUGE: return &InterTightBold52pt8bfr;
        }
    } else if (scale == FONT_SCALE_LARGE) {
        switch (font) {
            case EXTRA_SMALL: return &InterTight_SemiBold12pt8b;
            case TINY: return &InterTight_SemiBold14pt8b;
            case SMALL: return &InterTight_Bold16pt8b;
            case DAY_NUMBER: return &InterTight_Bold18pt8b;
            case MEDIUM: return &InterTight_Bold22pt8b;
            case LARGE: return &InterTight_Bold34pt8b;
            case EXTRA_LARGE:
            case HUGE: return &InterTightBold52pt8bfr;
        }
    }
    
    return getFont(font);
}

const GFXfont* Display102::getFont(Font font)
{
    switch (font) {
        case EXTRA_SMALL: return &InterTight_Bold8pt8b;
        case TINY: return &InterTight_SemiBold10pt8b;
        case SMALL: return &InterTight_SemiBold12pt8b;
        case DAY_NUMBER: return &InterTight_SemiBold14pt8b;
        case MEDIUM: return &InterTight_Bold16pt8b;
        case LARGE: return &InterTight_Bold24pt8b;
        case EXTRA_LARGE:
        case HUGE: return &InterTightBold52pt8bfr;
    }
    return &InterTight_SemiBold12pt8b;
}

// ============================================================================
// Text Rendering
// ============================================================================

Rectangle_t Display102::drawText(Font font, String text, int x, int y,
    Alignment horizontal, Alignment vertical,
    int margin_horizontal, int margin_vertical,
    int foregroundColor, int backgroundColor)
{
    String textCP = utf8tocp(text);
    const GFXfont *f = getFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    
    cp437(true);
    setFont(f);
    setTextWrap(false);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);
    
    switch (horizontal) {
        case LEADING: x = x + margin_horizontal - x1; break;
        case CENTER: x = x - w / 2 - x1; break;
        case TRAILING: x = x - w - margin_horizontal - x1; break;
    }
    
    switch (vertical) {
        case LEADING: y = y + h + margin_vertical - (y1 + h); break;
        case CENTER: {
            int16_t refX, refY;
            uint16_t refW, refH;
            getTextBounds("X", 0, 0, &refX, &refY, &refW, &refH);
            y = y + refH / 2;
        } break;
        case TRAILING: y = y - margin_vertical - (y1 + h); break;
    }
    
    if (backgroundColor >= 0) {
        fillRectRounded(x, y - h + 1, w, h - 1, 2, backgroundColor);
    }
    
    Rectangle_t rect;
    rect.x = x;
    rect.y = y - h + 1;
    rect.w = w;
    rect.h = h - 1;
    
    setTextColor(mapColor(foregroundColor));
    setCursor(x, y);
    print(textCP);
    
    return rect;
}

Rectangle_t Display102::drawTextScaled(Font font, String text, int x, int y,
    Alignment horizontal, Alignment vertical,
    int margin_horizontal, int margin_vertical,
    int foregroundColor, int backgroundColor)
{
    String textCP = utf8tocp(text);
    const GFXfont *f = getScaledFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    
    cp437(true);
    setFont(f);
    setTextWrap(false);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);
    
    switch (horizontal) {
        case LEADING: x = x + margin_horizontal - x1; break;
        case CENTER: x = x - w / 2 - x1; break;
        case TRAILING: x = x - w - margin_horizontal - x1; break;
    }
    
    switch (vertical) {
        case LEADING: y = y + margin_vertical - y1; break;
        case CENTER: {
            int16_t refX, refY;
            uint16_t refW, refH;
            getTextBounds("X", 0, 0, &refX, &refY, &refW, &refH);
            y = y + refH / 2;
        } break;
        case TRAILING: y = y - margin_vertical - (y1 + h); break;
    }
    
    if (backgroundColor >= 0) {
        fillRectRounded(x, y - h + 1, w, h - 1, 2, backgroundColor);
    }
    
    Rectangle_t rect;
    rect.x = x;
    rect.y = y - h + 1;
    rect.w = w;
    rect.h = h - 1;
    
    setTextColor(mapColor(foregroundColor));
    setCursor(x, y);
    print(textCP);
    
    return rect;
}

Rectangle_t Display102::drawTextMultiline(Font font, const char* lines[], int lineCount,
    int x, int y, int width, int height,
    Alignment horizontal, Alignment vertical, int foregroundColor)
{
    int lineHeight = getLineHeight(font);
    
    int actualLineCount = 0;
    for (int i = 0; i < lineCount; i++) {
        if (lines[i] != nullptr && strlen(lines[i]) > 0) {
            actualLineCount++;
        }
    }
    if (actualLineCount == 0) actualLineCount = 1;
    
    int baselineSpan = (actualLineCount - 1) * lineHeight;
    
    int startY;
    switch (vertical) {
        case LEADING:
            startY = y + lineHeight;
            break;
        case CENTER:
            startY = y + height / 2 - baselineSpan / 2;
            break;
        case TRAILING:
            startY = y + height - lineHeight / 2;
            if (actualLineCount > 1) startY -= baselineSpan;
            break;
        default:
            startY = y + height / 2 - baselineSpan / 2;
            break;
    }
    
    int textX;
    switch (horizontal) {
        case LEADING: textX = x; break;
        case CENTER: textX = x + width / 2; break;
        case TRAILING: textX = x + width; break;
        default: textX = x + width / 2; break;
    }
    
    Rectangle_t bounds = {x, startY - lineHeight, width, baselineSpan + lineHeight};
    int drawnLine = 0;
    for (int i = 0; i < lineCount; i++) {
        if (lines[i] != nullptr && strlen(lines[i]) > 0) {
            int lineY = startY + drawnLine * lineHeight;
            drawText(font, lines[i], textX, lineY, horizontal, LEADING, 0, 0, foregroundColor, -1);
            drawnLine++;
        }
    }
    
    return bounds;
}

Rectangle_t Display102::measureText(Font font, String text, int x, int y,
    Alignment horizontal, Alignment vertical,
    int margin_horizontal, int margin_vertical)
{
    String textCP = utf8tocp(text);
    const GFXfont *f = getFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    
    cp437(true);
    setFont(f);
    setTextWrap(false);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);
    
    uint16_t lineHeight = f->yAdvance;
    
    switch (horizontal) {
        case LEADING: x = x + margin_horizontal - x1; break;
        case CENTER: x = x - w / 2 - x1; break;
        case TRAILING: x = x - w - margin_horizontal - x1; break;
    }
    
    switch (vertical) {
        case LEADING: y = y + lineHeight + margin_vertical - (y1 + lineHeight); break;
        case CENTER: y = y + lineHeight / 2 - (y1 + lineHeight); break;
        case TRAILING: y = y - margin_vertical - (y1 + lineHeight); break;
    }
    
    Rectangle_t rect;
    rect.x = x;
    rect.y = y - lineHeight + 1;
    rect.w = w;
    rect.h = lineHeight - 1;
    
    return rect;
}

Rectangle_t Display102::measureTextScaled(Font font, String text, int x, int y,
    Alignment horizontal, Alignment vertical,
    int margin_horizontal, int margin_vertical)
{
    String textCP = utf8tocp(text);
    const GFXfont *f = getScaledFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    
    cp437(true);
    setFont(f);
    setTextWrap(false);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);
    
    uint16_t lineHeight = f->yAdvance;
    
    switch (horizontal) {
        case LEADING: x = x + margin_horizontal - x1; break;
        case CENTER: x = x - w / 2 - x1; break;
        case TRAILING: x = x - w - margin_horizontal - x1; break;
    }
    
    switch (vertical) {
        case LEADING: y = y + lineHeight + margin_vertical - (y1 + lineHeight); break;
        case CENTER: y = y + lineHeight / 2 - (y1 + lineHeight); break;
        case TRAILING: y = y - margin_vertical - (y1 + lineHeight); break;
    }
    
    Rectangle_t rect;
    rect.x = x;
    rect.y = y - lineHeight + 1;
    rect.w = w;
    rect.h = lineHeight - 1;
    
    return rect;
}

int Display102::getLineHeight(Font font)
{
    const GFXfont *f = getFont(font);
    return f->yAdvance;
}

int Display102::getLineHeightScaled(Font font)
{
    const GFXfont *f = getScaledFont(font);
    return f->yAdvance;
}

int Display102::getTextHeight(Font font, String text)
{
    String textCP = utf8tocp(text);
    const GFXfont *f = getFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    cp437(true);
    setFont(f);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);
    return h;
}

int Display102::getTextHeightScaled(Font font, String text)
{
    String textCP = utf8tocp(text);
    const GFXfont *f = getScaledFont(font);
    int16_t x1, y1;
    uint16_t w, h;
    cp437(true);
    setFont(f);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);
    return h;
}
