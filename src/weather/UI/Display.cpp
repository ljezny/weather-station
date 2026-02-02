#include <SPI.h>
#include <WiFi.h>
#include "Adafruit_GFX.h"

#include "Display.hpp"
#include "../Logging/Logging.hpp"
#include "gfxlatin2.h"

#include "InterTightBold8pt8bfr.h"
#include "InterTightSemiBold10pt8bfr.h"
#include "InterTightSemiBold12pt8bfr.h"
#include "InterTightBold16pt8bfr.h"
#include "InterTightBold24pt8bfr.h"

#define PIN_PWR_EN 2
#define PIN_BUSY 4
#define PIN_RST 16
#define PIN_DC 17
#define PIN_CS 5

Display::Display() : Adafruit_GFX(EPD_WIDTH, EPD_HEIGHT)
{
    blackFrameBuffer = NULL;
    redFrameBuffer = NULL;
}

Display::~Display()
{
}

void Display::beginDraw()
{
    LOGD("beginDraw: Allocating frame buffers, max alloc: " + String(ESP.getMaxAllocHeap()));
    if (blackFrameBuffer == NULL)
    {
        blackFrameBuffer = (uint8_t *)malloc(EPD_ARRAY * sizeof(uint8_t));
        if (blackFrameBuffer == NULL)
        {
            LOGD("Malloc failed for blackFrameBuffer");
        }
    }
    if (redFrameBuffer == NULL)
    {
        redFrameBuffer = (uint8_t *)malloc(EPD_ARRAY * sizeof(uint8_t));
        if (redFrameBuffer == NULL)
        {
            LOGD("Malloc failed for redFrameBuffer");
        }
    }
    LOGD("beginDraw: Buffers allocated, max alloc now: " + String(ESP.getMaxAllocHeap()));
}

void Display::endDraw()
{
    LOGD("endDraw: Freeing frame buffers");
    if (blackFrameBuffer != NULL)
    {
        free(blackFrameBuffer);
        blackFrameBuffer = NULL;
    }
    if (redFrameBuffer != NULL)
    {
        free(redFrameBuffer);
        redFrameBuffer = NULL;
    }
    LOGD("endDraw: Buffers freed, max alloc now: " + String(ESP.getMaxAllocHeap()));
}

Rectangle_t Display::drawText(Font font, String text, int x, int y, Alignment horizontal, Alignment vertical, int margin_horizontal, int margin_vertical, int foregroundColor, int backgroundColor)
{
    String textCP = utf8tocp(text);
    const GFXfont *f;
    switch (font)
    {
    case EXTRA_SMALL:
        f = &InterTight_Bold8pt8b;
        break;
    case TINY:
        f = &InterTight_SemiBold10pt8b;
        break;
    case SMALL:
        f = &InterTight_SemiBold12pt8b;
        break;
    case MEDIUM:
        f = &InterTight_Bold16pt8b;
        break;
    case LARGE:
        f = &InterTight_Bold24pt8b;
        break;
    default:
        f = &InterTight_SemiBold12pt8b;
        break;
    }
    int16_t x1, y1;
    uint16_t w, h;
    cp437(true);
    setFont(f);
    setTextWrap(false);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);

    switch (horizontal)
    {
    case LEADING:
        x = x + margin_horizontal - x1;
        break;
    case CENTER:
        x = x - w / 2 - x1;
        break;
    case TRAILING:
        x = x - w - margin_horizontal - x1;
        break;
    }

    switch (vertical)
    {
    case LEADING:
        y = y + h + margin_vertical - (y1 + h);
        break;
    case CENTER:
        {
            int16_t refX, refY;
            uint16_t refW, refH;
            getTextBounds("X", 0, 0, &refX, &refY, &refW, &refH);
            y = y + refH / 2;
        }
        break;
    case TRAILING:
        y = y - margin_vertical - (y1 + h);
        break;
    }
    if (backgroundColor >= 0)
    {
        fillRoundRect(x, y - h + 1, w, h - 1, 2, backgroundColor);
    }
    Rectangle_t rect;
    rect.x = x;
    rect.y = y - h + 1;
    rect.w = w;
    rect.h = h - 1;

    setTextColor(foregroundColor);
    setCursor(x, y);
    print(textCP);

    return rect;
}

Rectangle_t Display::measureText(Font font, String text, int x, int y, Alignment horizontal, Alignment vertical, int margin_horizontal, int margin_vertical)
{
    String textCP = utf8tocp(text);
    const GFXfont *f;
    switch (font)
    {
    case EXTRA_SMALL:
        f = &InterTight_Bold8pt8b;
        break;
    case TINY:
        f = &InterTight_SemiBold10pt8b;
        break;
    case SMALL:
        f = &InterTight_SemiBold12pt8b;
        break;
    case MEDIUM:
        f = &InterTight_Bold16pt8b;
        break;
    case LARGE:
        f = &InterTight_Bold24pt8b;
        break;
    default:
        f = &InterTight_SemiBold12pt8b;
        break;
    }
    int16_t x1, y1;
    uint16_t w, h;
    cp437(true);
    setFont(f);
    setTextWrap(false);
    getTextBounds(textCP, 0, 0, &x1, &y1, &w, &h);

    uint16_t lineHeight = f->yAdvance;

    switch (horizontal)
    {
    case LEADING:
        x = x + margin_horizontal - x1;
        break;
    case CENTER:
        x = x - w / 2 - x1;
        break;
    case TRAILING:
        x = x - w - margin_horizontal - x1;
        break;
    }

    switch (vertical)
    {
    case LEADING:
        y = y + lineHeight + margin_vertical - (y1 + lineHeight);
        break;
    case CENTER:
        y = y + lineHeight / 2 - (y1 + lineHeight);
        break;
    case TRAILING:
        y = y - margin_vertical - (y1 + lineHeight);
        break;
    }
    
    Rectangle_t rect;
    rect.x = x;
    rect.y = y - lineHeight + 1;
    rect.w = w;
    rect.h = lineHeight - 1;

    return rect;
}

int Display::getLineHeight(Font font)
{
    const GFXfont *f;
    switch (font)
    {
    case EXTRA_SMALL:
        f = &InterTight_Bold8pt8b;
        break;
    case TINY:
        f = &InterTight_SemiBold10pt8b;
        break;
    case SMALL:
        f = &InterTight_SemiBold12pt8b;
        break;
    case MEDIUM:
        f = &InterTight_Bold16pt8b;
        break;
    case LARGE:
        f = &InterTight_Bold24pt8b;
        break;
    default:
        f = &InterTight_SemiBold12pt8b;
        break;
    }
    return f->yAdvance;
}

void Display::drawImage(const uint8_t *data, int x, int y, int w, int h, Alignment horizontal, Alignment vertical, int margin)
{
    switch (horizontal)
    {
    case TRAILING:
        x = x - w - margin;
        break;
    case CENTER:
        x = x - w / 2;
        break;
    case LEADING:
        x = x + margin;
        break;
    }
    switch (vertical)
    {
    case TRAILING:
        y = y - h - margin;
        break;
    case CENTER:
        y = y - h / 2;
        break;
    case LEADING:
        y = y + margin;
        break;
    }
}

void Display::drawBitmapFromBuffer(const uint8_t *data, int x, int y, int w, int h, uint16_t color)
{
    // Draw 1-bit bitmap from buffer (MSB first, row-major order)
    for (int row = 0; row < h; row++)
    {
        for (int col = 0; col < w; col++)
        {
            int byteIndex = (row * ((w + 7) / 8)) + (col / 8);
            int bitIndex = 7 - (col % 8);
            if (data[byteIndex] & (1 << bitIndex))
            {
                drawPixel(x + col, y + row, color);
            }
        }
    }
}

void Display::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (blackFrameBuffer == NULL || redFrameBuffer == NULL)
    {
        return;
    }
    if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
        return;
    
#if PORTRAIT_MODE
    // Rotate coordinates: virtual (x, y) -> physical (y, EPD_PHYS_HEIGHT - 1 - x)
    int16_t phys_x = y;
    int16_t phys_y = EPD_PHYS_HEIGHT - 1 - x;
    uint16_t i = phys_x / 8 + phys_y * (EPD_PHYS_WIDTH / 8);
    int bit = 7 - phys_x % 8;
#else
    uint16_t i = x / 8 + y * (EPD_PHYS_WIDTH / 8);
    int bit = 7 - x % 8;
#endif

    blackFrameBuffer[i] = (blackFrameBuffer[i] & (0xFF ^ (1 << bit)));
    redFrameBuffer[i] = (redFrameBuffer[i] & (0xFF ^ (1 << bit)));
    if (color == COLOR_BLACK)
    {
        blackFrameBuffer[i] = (blackFrameBuffer[i] | (1 << bit));
    }
    else if (color == COLOR_RED)
    {
        redFrameBuffer[i] = (redFrameBuffer[i] | (1 << bit));
    }
}

void Display::fillRectRounded(int x, int y, int w, int h, int cornerRadius, int color)
{
    if (cornerRadius == 0)
    {
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

void Display::powerOn()
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
}

void Display::powerOff()
{
    writeCommand(0X50);
    writeData(0xf7);

    writeCommand(0X02);
    waitBusy();
    delay(100);
    writeCommand(0X07);
    writeData(0xA5);

    SPI.end();
    SPI.endTransaction();

    digitalWrite(PIN_PWR_EN, LOW);
}

void Display::init(bool partialMode)
{
    LOGD("Initializing display");
    digitalWrite(PIN_RST, LOW);
    delay(10);
    LOGD("Resetting");
    digitalWrite(PIN_RST, HIGH);
    delay(10);
    LOGD("Reset done");
    waitBusy();
    LOGD("Not busy");

    writeCommand(0xE0);
    writeData(0x02);
    writeCommand(0xE5);
    writeData(partialMode ? 0x6E : 0x5A);

    writeCommand(0x04);
    delay(100);
    waitBusy();

    writeCommand(0X00);
    writeData(partialMode ? 0x1F : 0x0F);

    writeCommand(0x61);
    writeData(EPD_PHYS_WIDTH / 256);
    writeData(EPD_PHYS_WIDTH % 256);
    writeData(EPD_PHYS_HEIGHT / 256);
    writeData(EPD_PHYS_HEIGHT % 256);

    writeCommand(0X15);
    writeData(0x00);

    writeCommand(0X50);
    if (partialMode)
    {
        writeData(0xA9);
    }
    else
    {
        writeData(0x11);
    }
    writeData(0x07);

    writeCommand(0X60);
    writeData(0x22);
}

void Display::clear()
{
    if (blackFrameBuffer == NULL || redFrameBuffer == NULL)
    {
        LOGD("Buffers not initialized.");
        return;
    }
    memset(blackFrameBuffer, 0, EPD_ARRAY);
    memset(redFrameBuffer, 0, EPD_ARRAY);
}

void Display::end()
{
    writeCommand(0x12);
    delay(1);
    waitBusy();
}

void Display::copyFrameBufferFullscreen()
{
    if (blackFrameBuffer == NULL || redFrameBuffer == NULL)
    {
        LOGD("Buffers not initialized.");
        return;
    }
    writeCommand(0x10);
    for (int i = 0; i < EPD_ARRAY; i++)
    {
        writeData(~blackFrameBuffer[i]);
    }
    writeCommand(0x13);
    for (int i = 0; i < EPD_ARRAY; i++)
    {
        writeData(redFrameBuffer[i]);
    }
}

int Display::getDisplayWidth()
{
    return EPD_WIDTH;
}

int Display::getDisplayHeight()
{
    return EPD_HEIGHT;
}

void Display::updateFullscreen()
{
    powerOn();
    init(false);
    setFullscreen();
    copyFrameBufferFullscreen();
    end();
    powerOff();
}

void Display::setFullscreen()
{
    writeCommand(0X00);
    writeData(0x0F);

    writeCommand(0x61);
    writeData(EPD_PHYS_WIDTH / 256);
    writeData(EPD_PHYS_WIDTH % 256);
    writeData(EPD_PHYS_HEIGHT / 256);
    writeData(EPD_PHYS_HEIGHT % 256);
}

void Display::writeSPI(unsigned char value)
{
    SPI.transfer(value);
}

void Display::writeData(unsigned char data)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    writeSPI(data);
    digitalWrite(PIN_CS, HIGH);
}

void Display::writeCommand(unsigned char command)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, LOW);
    writeSPI(command);
    digitalWrite(PIN_CS, HIGH);
}

void Display::waitBusy()
{
    while (digitalRead(PIN_BUSY) == 0)
    {
        delay(10);
    }
}
