#include "Screen.hpp"
#include "../Logging/Logging.hpp"
#include "../API/PngDecoder.hpp"
#include "CzechBorder.hpp"
#include "WeatherIcons.hpp"

// Globální ukazatel na display pro PNG callback
static Display* g_displayPtr = NULL;
static int g_mapX = 0, g_mapY = 0;
static int g_tileOffsetX = 0, g_tileOffsetY = 0;
static int g_clipX = 0, g_clipY = 0, g_clipW = 0, g_clipH = 0;
static int g_tileScale = 1;
int g_pixelScale = 1;  // Zvětšení pixelů (1 tile pixel = g_pixelScale screen pixels)

// Callback pro vykreslení řádku z PNG - s podporou zvětšení
static void pngLineCallback(int y, int width, uint16_t* lineBuffer, void* context)
{
    if (g_displayPtr == NULL) return;
    
    // Pro každý řádek tile pixelů vykreslíme g_pixelScale řádků na displeji
    for (int py = 0; py < g_pixelScale; py++)
    {
        int screenY = g_mapY + g_tileOffsetY + y * g_pixelScale + py;
        
        // Clip Y
        if (screenY < g_clipY || screenY >= g_clipY + g_clipH) continue;
        
        for (int i = 0; i < width; i++)
        {
            uint16_t color = lineBuffer[i];
            uint8_t intensity = PngDecoder::getIntensity(color);
            
            if (intensity > 0)
            {
                // Pro každý tile pixel vykreslíme g_pixelScale x g_pixelScale blok
                for (int px = 0; px < g_pixelScale; px++)
                {
                    int screenX = g_mapX + g_tileOffsetX + i * g_pixelScale + px;
                    
                    // Clip X
                    if (screenX < g_clipX || screenX >= g_clipX + g_clipW) continue;
                    
                    // Vykresli pixel podle intenzity
                    if (intensity >= 4)
                    {
                        // Velmi silné - červená
                        g_displayPtr->drawPixel(screenX, screenY, COLOR_RED);
                    }
                    else if (intensity >= 3)
                    {
                        // Silné - černá plná
                        g_displayPtr->drawPixel(screenX, screenY, COLOR_BLACK);
                    }
                    else if (intensity >= 2)
                    {
                        // Mírné - šachovnice
                        if ((screenX + screenY) % 2 == 0)
                        {
                            g_displayPtr->drawPixel(screenX, screenY, COLOR_BLACK);
                        }
                    }
                    else
                    {
                        // Slabé - řídké tečky
                        if ((screenX + screenY) % 3 == 0)
                        {
                            g_displayPtr->drawPixel(screenX, screenY, COLOR_BLACK);
                        }
                    }
                }
            }
        }
    }
}

WeatherScreen::WeatherScreen()
{
}

void WeatherScreen::drawBootScreen()
{
    display.beginDraw();
    display.clear();
    
    // Draw title centered
    display.drawText(LARGE, "Weather Radar", display.getDisplayWidth() / 2, display.getDisplayHeight() / 2 - 40, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    display.drawText(SMALL, "Česká republika", display.getDisplayWidth() / 2, display.getDisplayHeight() / 2, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    display.drawText(TINY, "Načítám...", display.getDisplayWidth() / 2, display.getDisplayHeight() / 2 + 50, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    display.updateFullscreen();
    display.endDraw();
}

void WeatherScreen::drawHeader(WeatherScreenData_t& data, const String& locationName)
{
    int headerHeight = 50;
    
    // Header background
    display.fillRect(0, 0, display.getDisplayWidth(), headerHeight, COLOR_BLACK);
    
    // Location name
    display.drawText(MEDIUM, locationName, 20, headerHeight / 2, 
                     LEADING, CENTER, 0, 0, COLOR_WHITE);
    
    // Current time
    if (data.currentTime > 0)
    {
        struct tm* timeinfo = localtime(&data.currentTime);
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);
        display.drawText(MEDIUM, String(timeStr), display.getDisplayWidth() - 20, headerHeight / 2, 
                         TRAILING, CENTER, 0, 0, COLOR_WHITE);
    }
}

void WeatherScreen::drawStatusBar(WeatherScreenData_t& data)
{
    int barY = display.getDisplayHeight() - 35;
    int barHeight = 35;
    
    // Status bar background
    display.fillRect(0, barY, display.getDisplayWidth(), barHeight, COLOR_BLACK);
    
    // Current time
    if (data.currentTime > 0)
    {
        struct tm* timeinfo = localtime(&data.currentTime);
        char dateStr[30];
        strftime(dateStr, sizeof(dateStr), "%d.%m.%Y %H:%M", timeinfo);
        display.drawText(TINY, String(dateStr), 15, barY + barHeight / 2, 
                         LEADING, CENTER, 0, 0, COLOR_WHITE);
    }
    
    // Battery level
    String batteryStr = String(data.batteryLevel) + "%";
    display.drawText(TINY, batteryStr, display.getDisplayWidth() - 15, barY + barHeight / 2, 
                     TRAILING, CENTER, 0, 0, COLOR_WHITE);
    
    // WiFi signal
    String wifiStr = "WiFi: " + String(data.wifiSignalLevel) + "%";
    display.drawText(TINY, wifiStr, display.getDisplayWidth() - 80, barY + barHeight / 2, 
                     TRAILING, CENTER, 0, 0, COLOR_WHITE);
}

void WeatherScreen::drawRadarLegend()
{
    // Legenda na pravé straně - pro landscape layout
    int legendX = display.getDisplayWidth() - 130;
    int legendY = 60;
    int boxSize = 18;
    int spacing = 8;
    
    // Legend title
    display.drawText(EXTRA_SMALL, "Intenzita:", legendX, legendY, 
                     LEADING, TRAILING, 0, 0, COLOR_BLACK);
    
    // Legend items - using patterns for e-ink
    // No rain - white
    int y0 = legendY + spacing;
    display.drawRect(legendX, y0, boxSize, boxSize, COLOR_BLACK);
    display.drawText(EXTRA_SMALL, "Bez srážek", legendX + boxSize + 5, y0 + boxSize/2, 
                     LEADING, CENTER, 0, 0, COLOR_BLACK);
    
    // Light rain - sparse dots
    int y1 = y0 + boxSize + spacing;
    display.drawRect(legendX, y1, boxSize, boxSize, COLOR_BLACK);
    for (int i = 0; i < 4; i++) {
        display.drawPixel(legendX + 4 + (i % 2) * 9, y1 + 4 + (i / 2) * 9, COLOR_BLACK);
    }
    display.drawText(EXTRA_SMALL, "Slabý", legendX + boxSize + 5, y1 + boxSize/2, 
                     LEADING, CENTER, 0, 0, COLOR_BLACK);
    
    // Moderate - checkered
    int y2 = y1 + boxSize + spacing;
    display.drawRect(legendX, y2, boxSize, boxSize, COLOR_BLACK);
    for (int i = 0; i < boxSize; i += 2) {
        for (int j = 0; j < boxSize; j += 2) {
            if ((i + j) % 4 == 0) {
                display.drawPixel(legendX + i, y2 + j, COLOR_BLACK);
            }
        }
    }
    display.drawText(EXTRA_SMALL, "Mírný", legendX + boxSize + 5, y2 + boxSize/2, 
                     LEADING, CENTER, 0, 0, COLOR_BLACK);
    
    // Heavy - dense
    int y3 = y2 + boxSize + spacing;
    display.fillRect(legendX, y3, boxSize, boxSize, COLOR_BLACK);
    display.drawText(EXTRA_SMALL, "Silný", legendX + boxSize + 5, y3 + boxSize/2, 
                     LEADING, CENTER, 0, 0, COLOR_BLACK);
    
    // Intense - red
    int y4 = y3 + boxSize + spacing;
    display.fillRect(legendX, y4, boxSize, boxSize, COLOR_RED);
    display.drawText(EXTRA_SMALL, "Přívalový", legendX + boxSize + 5, y4 + boxSize/2, 
                     LEADING, CENTER, 0, 0, COLOR_BLACK);
}

void WeatherScreen::drawRadarScreen(WeatherScreenData_t& data, RadarStorage& storage)
{
    LOGD("Drawing radar screen");
    
    display.beginDraw();
    display.clear();
    
    // Fullscreen layout s 5% marginem
    // Displej: 800x480
    int screenWidth = display.getDisplayWidth();   // 800
    int screenHeight = display.getDisplayHeight(); // 480
    
    int marginX = screenWidth * 5 / 100;   // 40px (5%)
    int marginY = screenHeight * 5 / 100;  // 24px (5%)
    
    int mapX = marginX;
    int mapY = marginY;
    int mapWidth = screenWidth - 2 * marginX;   // 720px
    int mapHeight = screenHeight - 2 * marginY; // 432px
    
    // Map border
    display.drawRect(mapX - 1, mapY - 1, mapWidth + 2, mapHeight + 2, COLOR_BLACK);
    
    // Parametry pro tile grid a obrys ČR
    // ČR se rozprostírá přibližně na: lon 12.1-18.9 (6.8°), lat 48.5-51.1 (2.6°)
    // Střed ČR: lon ~15.5, lat ~49.8
    // 
    // Pro zoom 6: 1 tile = 5.625° longitude, ~2.8° latitude (u 50°N)
    // Pro zoom 7: 1 tile = 2.8125° longitude, ~1.4° latitude
    // 
    // Zoom 7 je lepší pro detail ČR - potřebujeme ~2.5 tiles horizontálně
    // Ale máme jen 2x2 grid, takže použijeme zoom 6 s menším scale
    
    int zoom = 6;
    int baseTileX = 34;  // Z getCzechTileCoords pro zoom 6
    int baseTileY = 21;
    
    // ČR v tile souřadnicích zoom 6:
    // - zabírá cca 170px horizontálně a 120px vertikálně v 256px tile
    // - potřebujeme scale tak, aby se to vešlo do viewportu
    // 
    // Originální 2x2 tile grid = 512x512 px
    // ČR je cca v prostředních 40% horizontálně a 25% vertikálně
    // ČR zabírá cca 200x130 px z 512x512
    // 
    // Pro 720x432 viewport chceme ČR roztáhnout:
    // Scale = 1 by dalo 200x130 pro ČR
    // Potřebujeme škálovat nahoru (scale < 1)
    // Ale scale musí být int, takže místo toho použijeme vyšší zoom
    
    // S zoom 6 a scale 1: ČR je cca 200x130px
    // Chceme aby ČR zabírala většinu z 720x432
    // Poměr: 720/200 = 3.6x, 432/130 = 3.3x
    // Nejmenší je 3.3x - takže použijeme scale že tile pixely jsou 3x větší
    // To ale nejde přímo, musíme použít jiný přístup
    
    // Použijeme "sub-pixel" rendering - každý pixel z tile vykreslíme jako NxN blok
    int pixelScale = 3;  // Každý pixel radarových dat = 3x3 pixely na displeji
    
    // S pixelScale=2: 512*2 = 1024px, stále příliš velké
    // Potřebujeme vykreslit jen výřez kolem ČR
    // 
    // ČR souřadnice (GPS): lon 12.09 - 18.86, lat 48.55 - 51.11
    // To odpovídá v tile pixelech na zoom 6 (base tile 34,21):
    // X: cca 8 - 248 px (šířka ~240px)  
    // Y: cca 145 - 295 px (výška ~150px)
    //
    // S pixelScale 2: 240*2=480, 150*2=300 - to se dobře vejde do 720x432 s marginem
    
    // Přesnější hodnoty ČR v tile pixelech (zoom 6, base 34,21)
    int czStartX = 8;     // Pixel X kde začíná ČR v rámci 512px gridu (lon 12.09)
    int czStartY = 145;   // Pixel Y kde začíná ČR v rámci 512px gridu (lat 51.11)
    int czWidth = 240;    // Šířka ČR v tile pixelech
    int czHeight = 150;   // Výška ČR v tile pixelech
    
    // Snížíme pixelScale na 2 pro lepší fit
    pixelScale = 2;
    
    // Vycentrování v mapovém viewportu
    int scaledCzWidth = czWidth * pixelScale;   // 480
    int scaledCzHeight = czHeight * pixelScale; // 300
    int offsetX = (mapWidth - scaledCzWidth) / 2 - czStartX * pixelScale;
    int offsetY = (mapHeight - scaledCzHeight) / 2 - czStartY * pixelScale;
    
    // Pro kompatibilitu se stávajícím kódem
    int tileScale = 1;  // Neškálujeme dolů, ale používáme pixelScale pro zvětšení
    int tileDisplaySize = 256 * pixelScale; // Každá tile je nyní 512px
    
    // PRVNÍ: Vykresli obrys České republiky červeně s tlustou čarou (5px)
    // Upravíme CzechBorder pro pixelScale
    {
        int prevX = -1, prevY = -1;
        extern void latLonToTilePixel(double lat, double lon, int zoom, int baseTileX, int baseTileY, int& px, int& py);
        extern const double CZ_BORDER_GPS[][2];
        extern const int CZ_BORDER_POINTS;
        
        const int LINE_THICKNESS = 5;  // Tloušťka čáry v pixelech
        
        for (int i = 0; i < CZ_BORDER_POINTS; i++)
        {
            int px, py;
            latLonToTilePixel(CZ_BORDER_GPS[i][1], CZ_BORDER_GPS[i][0], zoom, baseTileX, baseTileY, px, py);
            
            int screenX = mapX + offsetX + px * pixelScale;
            int screenY = mapY + offsetY + py * pixelScale;
            
            if (prevX >= 0 && prevY >= 0)
            {
                // Kreslení tlusté čáry - více čar vedle sebe
                int dx = abs(screenX - prevX);
                int dy = abs(screenY - prevY);
                int halfThick = LINE_THICKNESS / 2;
                
                if (dx >= dy) {
                    // Horizontální nebo diagonální - posun v Y
                    for (int t = -halfThick; t <= halfThick; t++) {
                        display.drawLine(prevX, prevY + t, screenX, screenY + t, COLOR_RED);
                    }
                } else {
                    // Vertikální - posun v X
                    for (int t = -halfThick; t <= halfThick; t++) {
                        display.drawLine(prevX + t, prevY, screenX + t, screenY, COLOR_RED);
                    }
                }
            }
            
            prevX = screenX;
            prevY = screenY;
        }
    }
    
    // DRUHÉ: Přes obrys vykresli radarové dlaždice srážek
    if (data.hasRadarData && storage.isInitialized())
    {
        int tileCount = storage.getStoredTileCount();
        LOGD("Decoding and drawing " + String(tileCount) + " tiles");
        
        // Nastav globální kontext pro PNG callback
        g_displayPtr = &display;
        g_mapX = mapX;
        g_mapY = mapY;
        g_clipX = mapX;
        g_clipY = mapY;
        g_clipW = mapWidth;
        g_clipH = mapHeight;
        g_tileScale = 1;  // Bez zmenšení
        
        // Pro pixelScale používáme globální proměnnou
        g_pixelScale = pixelScale;
        
        PngDecoder decoder;
        
        // Dekóduj a vykresli každou dlaždici
        for (int t = 0; t < tileCount && t < 4; t++)
        {
            String tilePath = storage.getTilePath(t);
            
            // Pozice dlaždice v gridu 2x2
            int tileGridX = t % 2;
            int tileGridY = t / 2;
            
            g_tileOffsetX = offsetX + tileGridX * 256 * pixelScale;
            g_tileOffsetY = offsetY + tileGridY * 256 * pixelScale;
            
            LOGD("Decoding tile " + String(t) + " at offset " + 
                 String(g_tileOffsetX) + "," + String(g_tileOffsetY));
            
            if (!decoder.decodeFromFile(tilePath.c_str(), pngLineCallback, NULL,
                                        0, 0, g_clipX, g_clipY, g_clipW, g_clipH))
            {
                LOGD("Failed to decode tile " + String(t));
            }
        }
        
        g_displayPtr = NULL;
        g_pixelScale = 1;  // Reset
    }
    else
    {
        // No radar data available - nakresli alespoň obrys (už je nakreslený výše)
        int czX = mapX + mapWidth / 2;
        int czY = mapY + mapHeight / 2;
        display.drawText(SMALL, "Radar data není k dispozici", czX, czY, 
                         CENTER, CENTER, 0, 0, COLOR_BLACK);
    }
    
    // Minimální info v rohu - čas radaru
    if (data.radarTimestamp > 0)
    {
        struct tm* timeinfo = localtime(&data.radarTimestamp);
        char timeStr[30];
        strftime(timeStr, sizeof(timeStr), "Radar: %H:%M", timeinfo);
        display.drawText(TINY, String(timeStr), screenWidth - marginX, marginY / 2 + 5, 
                         TRAILING, CENTER, 0, 0, COLOR_BLACK);
    }
    
    display.updateFullscreen();
    display.endDraw();
    
    LOGD("Radar screen drawn");
}

void WeatherScreen::drawStatusScreen(String message)
{
    display.beginDraw();
    display.clear();
    
    display.drawText(MEDIUM, message, display.getDisplayWidth() / 2, display.getDisplayHeight() / 2, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    display.updateFullscreen();
    display.endDraw();
}

void WeatherScreen::drawErrorScreen(String error)
{
    display.beginDraw();
    display.clear();
    
    display.drawText(LARGE, "Chyba", display.getDisplayWidth() / 2, display.getDisplayHeight() / 2 - 40, 
                     CENTER, CENTER, 0, 0, COLOR_RED);
    display.drawText(SMALL, error, display.getDisplayWidth() / 2, display.getDisplayHeight() / 2 + 20, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    display.drawText(TINY, "Stiskněte RESET pro restart", display.getDisplayWidth() / 2, display.getDisplayHeight() / 2 + 60, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    display.updateFullscreen();
    display.endDraw();
}

void WeatherScreen::drawConfigScreen(String ssid, String ip)
{
    display.beginDraw();
    display.clear();
    
    display.drawText(LARGE, "Nastavení", display.getDisplayWidth() / 2, 60, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    display.drawText(MEDIUM, "Připojte se k WiFi:", display.getDisplayWidth() / 2, 140, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    display.drawText(LARGE, ssid, display.getDisplayWidth() / 2, 190, 
                     CENTER, CENTER, 0, 0, COLOR_RED);
    
    display.drawText(MEDIUM, "Nebo otevřete:", display.getDisplayWidth() / 2, 280, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    display.drawText(LARGE, "http://" + ip, display.getDisplayWidth() / 2, 330, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    display.drawText(SMALL, "Zadejte lokaci a WiFi údaje", display.getDisplayWidth() / 2, 420, 
                     CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    display.updateFullscreen();
    display.endDraw();
}

// ================== NEW WEATHER SCREEN LAYOUT ==================

void WeatherScreen::drawWeatherScreen(WeatherScreenData_t& data, RadarStorage& storage, WeatherData_t& weather)
{
    LOGD("Drawing weather screen with new layout");
    
    display.beginDraw();
    display.clear();
    
    // Layout constants for 800x480 display
    int screenWidth = display.getDisplayWidth();   // 800
    int screenHeight = display.getDisplayHeight(); // 480
    int headerHeight = 40;
    int statusBarHeight = 25;
    int padding = 8;
    
    // Content area
    int contentY = headerHeight;
    int contentHeight = screenHeight - headerHeight - statusBarHeight;
    
    // Left column: Radar (1/4 width) + Current weather
    int leftColWidth = screenWidth / 4;  // 200px
    
    // Right column: Hourly + Daily (3/4 width)
    int rightColX = leftColWidth;
    int rightColWidth = screenWidth - leftColWidth;  // 600px
    
    // Draw header with location name
    drawHeader(data, weather.locationName);
    
    // === LEFT COLUMN ===
    
    // Radar mini map (top-left quarter)
    int radarHeight = contentHeight / 2;
    drawRadarMini(data, storage, 0, contentY, leftColWidth, radarHeight);
    
    // Current weather (below radar)
    int currentY = contentY + radarHeight;
    int currentHeight = contentHeight - radarHeight;
    drawCurrentWeather(weather, 0, currentY, leftColWidth, currentHeight);
    
    // === RIGHT COLUMN ===
    
    // Hourly forecast (top-right, about 1/3 of right column height)
    int hourlyHeight = contentHeight / 3;
    drawHourlyForecast(weather, rightColX, contentY, rightColWidth, hourlyHeight);
    
    // Daily forecast (bottom-right, remaining 2/3)
    int dailyY = contentY + hourlyHeight;
    int dailyHeight = contentHeight - hourlyHeight;
    drawDailyForecast(weather, rightColX, dailyY, rightColWidth, dailyHeight);
    
    // Draw status bar at bottom
    drawStatusBar(data);
    
    display.updateFullscreen();
    display.endDraw();
    
    LOGD("Weather screen drawn");
}

void WeatherScreen::drawRadarMini(WeatherScreenData_t& data, RadarStorage& storage, int x, int y, int w, int h)
{
    // Border
    display.drawRect(x, y, w, h, COLOR_BLACK);
    
    // Title
    display.drawText(EXTRA_SMALL, "Radar srážek", x + w / 2, y + 10, CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    int mapX = x + 4;
    int mapY = y + 20;
    int mapW = w - 8;
    int mapH = h - 30;
    
    // Map border
    display.drawRect(mapX, mapY, mapW, mapH, COLOR_BLACK);
    
    if (data.hasRadarData && storage.isInitialized())
    {
        // Simplified radar display for mini view
        int zoom = 6;
        int baseTileX = 34;
        int baseTileY = 21;
        
        // For mini view, use smaller scale
        int pixelScale = 1;  // 1:1 mapping, we'll crop to fit
        
        // ČR souřadnice v tile pixelech
        int czStartX = 8;
        int czStartY = 145;
        int czWidth = 240;
        int czHeight = 150;
        
        // Scale to fit in mapW x mapH
        float scaleX = (float)mapW / czWidth;
        float scaleY = (float)mapH / czHeight;
        float scale = min(scaleX, scaleY);
        
        int scaledW = czWidth * scale;
        int scaledH = czHeight * scale;
        int offsetX = (mapW - scaledW) / 2;
        int offsetY = (mapH - scaledH) / 2;
        
        // Draw simplified border of Czech Republic
        extern void latLonToTilePixel(double lat, double lon, int zoom, int baseTileX, int baseTileY, int& px, int& py);
        extern const double CZ_BORDER_GPS[][2];
        extern const int CZ_BORDER_POINTS;
        
        int prevX = -1, prevY = -1;
        for (int i = 0; i < CZ_BORDER_POINTS; i += 3)  // Every 3rd point for simplified outline
        {
            int px, py;
            latLonToTilePixel(CZ_BORDER_GPS[i][1], CZ_BORDER_GPS[i][0], zoom, baseTileX, baseTileY, px, py);
            
            int screenX = mapX + offsetX + (px - czStartX) * scale;
            int screenY = mapY + offsetY + (py - czStartY) * scale;
            
            if (prevX >= 0 && prevY >= 0)
            {
                display.drawLine(prevX, prevY, screenX, screenY, COLOR_RED);
            }
            
            prevX = screenX;
            prevY = screenY;
        }
        
        // Radar time
        if (data.radarTimestamp > 0)
        {
            struct tm* timeinfo = localtime(&data.radarTimestamp);
            char timeStr[10];
            strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);
            display.drawText(EXTRA_SMALL, String(timeStr), x + w - 5, y + h - 8, TRAILING, CENTER, 0, 0, COLOR_BLACK);
        }
    }
    else
    {
        display.drawText(EXTRA_SMALL, "N/A", x + w / 2, y + h / 2, CENTER, CENTER, 0, 0, COLOR_BLACK);
    }
}

void WeatherScreen::drawCurrentWeather(WeatherData_t& weather, int x, int y, int w, int h)
{
    // Border
    display.drawRect(x, y, w, h, COLOR_BLACK);
    
    if (!weather.valid) {
        display.drawText(SMALL, "Načítání...", x + w / 2, y + h / 2, CENTER, CENTER, 0, 0, COLOR_BLACK);
        return;
    }
    
    int centerX = x + w / 2;
    int iconSize = 48;
    
    // Weather icon
    WeatherIcons::drawIconForCode(display, weather.current.weatherCode, 
                                  centerX, y + 35, iconSize, weather.current.isDay);
    
    // Temperature (big)
    String tempStr = String((int)round(weather.current.temperature)) + "°";
    display.drawText(LARGE, tempStr, centerX, y + 85, CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    // Feels like
    String feelsLike = "Pocitově " + String((int)round(weather.current.apparentTemperature)) + "°";
    display.drawText(EXTRA_SMALL, feelsLike, centerX, y + 115, CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    // Weather description
    String desc = OpenMeteoAPI::getWeatherDescription(weather.current.weatherCode);
    display.drawText(TINY, desc, centerX, y + 135, CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    // Additional info
    int infoY = y + 160;
    int lineH = 18;
    
    // Humidity
    display.drawText(EXTRA_SMALL, "Vlhkost: " + String(weather.current.humidity) + "%", 
                     centerX, infoY, CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    // Wind
    String windStr = "Vítr: " + String((int)weather.current.windSpeed) + " km/h";
    display.drawText(EXTRA_SMALL, windStr, centerX, infoY + lineH, CENTER, CENTER, 0, 0, COLOR_BLACK);
    
    // Precipitation if any
    if (weather.current.precipitation > 0)
    {
        String precipStr = "Srážky: " + String(weather.current.precipitation, 1) + " mm";
        display.drawText(EXTRA_SMALL, precipStr, centerX, infoY + lineH * 2, CENTER, CENTER, 0, 0, COLOR_BLACK);
    }
}

void WeatherScreen::drawHourlyForecast(WeatherData_t& weather, int x, int y, int w, int h)
{
    // Border
    display.drawRect(x, y, w, h, COLOR_BLACK);
    
    // Title
    display.drawText(SMALL, "Hodinová předpověď", x + 10, y + 15, LEADING, CENTER, 0, 0, COLOR_BLACK);
    
    if (!weather.valid || weather.hourlyCount == 0) {
        display.drawText(SMALL, "Načítání...", x + w / 2, y + h / 2, CENTER, CENTER, 0, 0, COLOR_BLACK);
        return;
    }
    
    // Show 8 hours
    int hoursToShow = min(8, weather.hourlyCount);
    int colWidth = (w - 20) / hoursToShow;
    int startX = x + 10 + colWidth / 2;
    int iconY = y + 50;
    int tempY = y + 90;
    int precipY = y + 110;
    int iconSize = 28;
    
    for (int i = 0; i < hoursToShow; i++)
    {
        int colX = startX + i * colWidth;
        HourlyForecast_t& hour = weather.hourly[i];
        
        // Hour
        String hourStr = String(hour.hour) + ":00";
        if (i == 0) hourStr = "Teď";
        display.drawText(EXTRA_SMALL, hourStr, colX, y + 32, CENTER, CENTER, 0, 0, COLOR_BLACK);
        
        // Icon
        bool isDay = (hour.hour >= 6 && hour.hour < 21);
        WeatherIcons::drawIconForCode(display, hour.weatherCode, colX, iconY, iconSize, isDay);
        
        // Temperature
        String temp = String((int)round(hour.temperature)) + "°";
        display.drawText(TINY, temp, colX, tempY, CENTER, CENTER, 0, 0, COLOR_BLACK);
        
        // Precipitation probability if > 0
        if (hour.precipitationProbability > 0)
        {
            String precip = String(hour.precipitationProbability) + "%";
            display.drawText(EXTRA_SMALL, precip, colX, precipY, CENTER, CENTER, 0, 0, COLOR_BLACK);
        }
    }
}

void WeatherScreen::drawDailyForecast(WeatherData_t& weather, int x, int y, int w, int h)
{
    // Border
    display.drawRect(x, y, w, h, COLOR_BLACK);
    
    // Title
    display.drawText(SMALL, "Týdenní předpověď", x + 10, y + 15, LEADING, CENTER, 0, 0, COLOR_BLACK);
    
    if (!weather.valid || weather.dailyCount == 0) {
        display.drawText(SMALL, "Načítání...", x + w / 2, y + h / 2, CENTER, CENTER, 0, 0, COLOR_BLACK);
        return;
    }
    
    // Show 7 days in rows
    int daysToShow = min(7, weather.dailyCount);
    int rowHeight = (h - 35) / daysToShow;
    int startY = y + 35;
    int iconSize = 24;
    
    // Column positions
    int dayNameX = x + 50;
    int iconX = x + 120;
    int tempMaxX = x + 200;
    int tempMinX = x + 270;
    int precipX = x + 350;
    int descX = x + 450;
    
    for (int i = 0; i < daysToShow; i++)
    {
        int rowY = startY + i * rowHeight + rowHeight / 2;
        DailyForecast_t& day = weather.daily[i];
        
        // Day name
        String dayName = (i == 0) ? "Dnes" : OpenMeteoAPI::getDayNameShort(day.dayOfWeek);
        String dateStr = String(day.dayOfMonth) + "." + String(day.month) + ".";
        display.drawText(TINY, dayName, dayNameX, rowY - 8, CENTER, CENTER, 0, 0, COLOR_BLACK);
        display.drawText(EXTRA_SMALL, dateStr, dayNameX, rowY + 8, CENTER, CENTER, 0, 0, COLOR_BLACK);
        
        // Icon
        WeatherIcons::drawIconForCode(display, day.weatherCode, iconX, rowY, iconSize, true);
        
        // Max temp (bold/red)
        String maxTemp = String((int)round(day.tempMax)) + "°";
        display.drawText(SMALL, maxTemp, tempMaxX, rowY, CENTER, CENTER, 0, 0, COLOR_BLACK);
        
        // Min temp (smaller)
        String minTemp = String((int)round(day.tempMin)) + "°";
        display.drawText(TINY, minTemp, tempMinX, rowY, CENTER, CENTER, 0, 0, COLOR_BLACK);
        
        // Precipitation
        if (day.precipitationProbability > 0)
        {
            String precipStr = String(day.precipitationProbability) + "%";
            if (day.precipitationSum > 0)
            {
                precipStr += " " + String(day.precipitationSum, 1) + "mm";
            }
            display.drawText(EXTRA_SMALL, precipStr, precipX, rowY, CENTER, CENTER, 0, 0, COLOR_BLACK);
        }
        
        // Short description
        String desc = OpenMeteoAPI::getWeatherDescription(day.weatherCode);
        if (desc.length() > 15) desc = desc.substring(0, 14) + ".";
        display.drawText(EXTRA_SMALL, desc, descX, rowY, LEADING, CENTER, 0, 0, COLOR_BLACK);
        
        // Separator line
        if (i < daysToShow - 1)
        {
            display.drawLine(x + 10, startY + (i + 1) * rowHeight, x + w - 10, startY + (i + 1) * rowHeight, COLOR_BLACK);
        }
    }
}
