#pragma once
#include <Arduino.h>
#include "Adafruit_GFX.h"

enum Alignment
{
    LEADING,
    TRAILING,
    CENTER
};

enum Font {
    EXTRA_SMALL,
    TINY,
    SMALL,
    MEDIUM,
    LARGE
};

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rectangle_t;

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800

// Physical display dimensions
#define EPD_PHYS_WIDTH 800
#define EPD_PHYS_HEIGHT 480

// Landscape mode (no rotation)
#define PORTRAIT_MODE 0

#if PORTRAIT_MODE
    #define EPD_WIDTH EPD_PHYS_HEIGHT
    #define EPD_HEIGHT EPD_PHYS_WIDTH
#else
    #define EPD_WIDTH EPD_PHYS_WIDTH
    #define EPD_HEIGHT EPD_PHYS_HEIGHT
#endif

#define EPD_ARRAY (EPD_PHYS_WIDTH * EPD_PHYS_HEIGHT / 8)

class Display : public Adafruit_GFX
{
public:
    Display();
    ~Display();
    void beginDraw();
    void endDraw();
    void clear();
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    Rectangle_t drawText(Font font, String text, int x, int y, Alignment horizontal, Alignment vertical, int margin_horizontal, int margin_vertical, int foregroundColor, int backgroundColor = -1);
    Rectangle_t measureText(Font font, String text, int x, int y, Alignment horizontal, Alignment vertical, int margin_horizontal, int margin_vertical);
    void drawImage(const uint8_t *data, int x, int y, int w, int h, Alignment horizontal, Alignment vertical, int margin);
    void drawBitmapFromBuffer(const uint8_t *data, int x, int y, int w, int h, uint16_t color);
    void fillRectRounded(int x, int y, int w, int h, int cornerRadius, int color);
    int getDisplayWidth();
    int getDisplayHeight();
    int getLineHeight(Font font);
    void updateFullscreen();

private:
    uint8_t *blackFrameBuffer;
    uint8_t *redFrameBuffer;

    void writeSPI(unsigned char value);
    void writeData(unsigned char data);
    void writeCommand(unsigned char command);
    void waitBusy();

    void powerOn();
    void powerOff();
    void init(bool partialMode);
    void copyFrameBufferFullscreen();
    void end();
    void setFullscreen();
};
