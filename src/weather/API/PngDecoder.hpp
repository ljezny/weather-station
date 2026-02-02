#pragma once
#include <Arduino.h>
#include <PNGdec.h>
#include <LittleFS.h>
#include "../Logging/Logging.hpp"

// Callback funkce pro čtení PNG z LittleFS
static void* pngOpen(const char* filename, int32_t* size)
{
    File file = LittleFS.open(filename, FILE_READ);
    if (!file)
    {
        return NULL;
    }
    *size = file.size();
    
    // Alokuj handle a ulož File objekt
    File* pFile = new File(file);
    return pFile;
}

static void pngClose(void* handle)
{
    File* pFile = (File*)handle;
    if (pFile)
    {
        pFile->close();
        delete pFile;
    }
}

static int32_t pngRead(PNGFILE* pFile, uint8_t* buffer, int32_t length)
{
    File* file = (File*)pFile->fHandle;
    return file->read(buffer, length);
}

static int32_t pngSeek(PNGFILE* pFile, int32_t position)
{
    File* file = (File*)pFile->fHandle;
    return file->seek(position) ? position : -1;
}

// Struktura pro předávání kontextu do draw callbacku
typedef struct {
    void* displayPtr;
    int offsetX;
    int offsetY;
    int clipX;
    int clipY;
    int clipW;
    int clipH;
} PngDrawContext_t;

// Globální kontext
static PngDrawContext_t* g_pngContext = NULL;

// Forward deklarace pro Display
class Display;

// Převod barvy na intenzitu srážek (0-4)
static uint8_t colorToRadarIntensity(uint16_t color565)
{
    // RGB565: RRRRRGGGGGGBBBBB
    uint8_t r = ((color565 >> 11) & 0x1F) << 3;
    uint8_t g = ((color565 >> 5) & 0x3F) << 2;
    uint8_t b = (color565 & 0x1F) << 3;
    
    // Průhledné/bílé pixely = žádné srážky
    if (r > 240 && g > 240 && b > 240) return 0;
    
    // RainViewer barevná škála:
    // Fialová/magenta = velmi silné (4)
    if (r > 180 && b > 150 && g < 100) return 4;
    // Červená = velmi silné (4)
    if (r > 200 && g < 80 && b < 80) return 4;
    // Oranžová = silné (3)
    if (r > 200 && g > 80 && g < 180 && b < 80) return 3;
    // Žlutá = mírné (2)
    if (r > 200 && g > 180 && b < 100) return 2;
    // Zelená = slabé (1)
    if (g > 150 && r < 150 && b < 150) return 1;
    // Modrá/tyrkysová = slabé (1)
    if (b > 150 || (g > 150 && b > 100)) return 1;
    
    // Ostatní barevné pixely = slabé srážky
    if (r > 50 || g > 50 || b > 50) return 1;
    
    return 0;
}

class PngDecoder
{
public:
    PNG png;
    
    // Dekóduj PNG a vykresli přímo na display
    // Callback je volán pro každý řádek
    typedef void (*LineDrawCallback)(int y, int width, uint16_t* lineBuffer, void* context);
    
    bool decodeFromFile(const char* filename, LineDrawCallback callback, void* context,
                        int offsetX, int offsetY, int clipX, int clipY, int clipW, int clipH)
    {
        LOGD("Decoding PNG: " + String(filename));
        
        // Ulož kontext
        drawCallback = callback;
        callbackContext = context;
        drawOffsetX = offsetX;
        drawOffsetY = offsetY;
        drawClipX = clipX;
        drawClipY = clipY;
        drawClipW = clipW;
        drawClipH = clipH;
        
        currentDecoder = this;
        
        int rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDrawLine);
        if (rc != PNG_SUCCESS)
        {
            LOGD("PNG open failed: " + String(rc));
            currentDecoder = NULL;
            return false;
        }
        
        LOGD("PNG: " + String(png.getWidth()) + "x" + String(png.getHeight()) + 
             " bpp=" + String(png.getBpp()) + " type=" + String(png.getPixelType()));
        
        rc = png.decode(NULL, 0);
        png.close();
        currentDecoder = NULL;
        
        if (rc != PNG_SUCCESS)
        {
            LOGD("PNG decode failed: " + String(rc));
            return false;
        }
        
        return true;
    }
    
    // Statický callback wrapper
    static int pngDrawLine(PNGDRAW* pDraw)
    {
        if (currentDecoder == NULL) return 0;
        currentDecoder->handleDrawLine(pDraw);
        return 1;
    }
    
    void handleDrawLine(PNGDRAW* pDraw)
    {
        if (drawCallback == NULL) return;
        
        int y = pDraw->y + drawOffsetY;
        
        // Clip vertikálně
        if (y < drawClipY || y >= drawClipY + drawClipH) return;
        
        // Alokuj buffer dynamicky místo na stacku
        uint16_t* lineBuffer = (uint16_t*)malloc(pDraw->iWidth * sizeof(uint16_t));
        if (lineBuffer == NULL) return;
        
        // Získej řádek jako RGB565
        png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);
        
        // Zavolej callback
        drawCallback(y, pDraw->iWidth, lineBuffer, callbackContext);
        
        free(lineBuffer);
    }
    
    // Získej intenzitu srážek z barvy
    static uint8_t getIntensity(uint16_t color565)
    {
        return colorToRadarIntensity(color565);
    }

private:
    static PngDecoder* currentDecoder;
    LineDrawCallback drawCallback;
    void* callbackContext;
    int drawOffsetX, drawOffsetY;
    int drawClipX, drawClipY, drawClipW, drawClipH;
};

// Inicializace statické proměnné
PngDecoder* PngDecoder::currentDecoder = NULL;
