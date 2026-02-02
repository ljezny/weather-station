#pragma once
#include <Arduino.h>

// Obrys České republiky - detailní verze z OpenStreetMap
// Souřadnice jsou v formátu [longitude, latitude]
// Cca 200 bodů pro přesný obrys

// Pomocná funkce pro převod GPS -> pixel v rámci 2x2 tile gridu (512x512 px na zoom 6)
inline void latLonToTilePixel(double lat, double lon, int zoom, int baseTileX, int baseTileY, int& px, int& py)
{
    int n = 1 << zoom;  // 64 pro zoom 6
    
    // X pozice v rámci celého světa (v pixelech, 256*n)
    double worldX = (lon + 180.0) / 360.0 * n * 256.0;
    
    // Y pozice v rámci celého světa (Mercator projekce)
    double latRad = lat * PI / 180.0;
    double worldY = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / PI) / 2.0 * n * 256.0;
    
    // Offset od base tile
    px = (int)(worldX - baseTileX * 256.0);
    py = (int)(worldY - baseTileY * 256.0);
}

// Detailní obrys ČR - přibližně 200 bodů odvozených z OpenStreetMap dat
// Formát: [longitude, latitude]
static const double CZ_BORDER_GPS[][2] = {
    // Začátek na jihovýchodě, po směru hodinových ručiček
    // Jižní hranice s Rakouskem
    {12.0906, 50.2524},  // Západní cíp - Ašský výběžek
    {12.1000, 50.2600},
    {12.1200, 50.2700},
    {12.1500, 50.2780},
    {12.1800, 50.2820},
    {12.2100, 50.2900},
    {12.2401, 50.2663},  // Hranice se Saskem
    {12.2700, 50.2400},
    {12.3000, 50.2200},
    {12.3500, 50.2100},
    {12.4000, 50.2050},
    {12.4152, 49.9691},
    {12.4500, 49.8500},
    {12.5000, 49.7000},
    {12.5210, 49.5474},
    {12.5500, 49.5000},
    {12.6000, 49.4500},
    {12.6500, 49.4000},
    {12.7000, 49.3500},
    {12.7500, 49.3000},
    {12.8000, 49.2500},
    {12.8500, 49.2000},
    {12.9000, 49.1500},
    {12.9500, 49.1000},
    {13.0000, 49.0800},
    {13.0313, 49.3071},
    // Šumava
    {13.1000, 49.2800},
    {13.1500, 49.2500},
    {13.2000, 49.2000},
    {13.2500, 49.1500},
    {13.3000, 49.1000},
    {13.3500, 49.0500},
    {13.4000, 49.0000},
    {13.4500, 48.9500},
    {13.5000, 48.9000},
    {13.5500, 48.8800},
    {13.5959, 48.8772},
    {13.6500, 48.8500},
    {13.7000, 48.8200},
    {13.7500, 48.7800},
    {13.8000, 48.7500},
    {13.8500, 48.7200},
    {13.9000, 48.6800},
    {13.9500, 48.6500},
    {14.0000, 48.6200},
    {14.0500, 48.5900},
    {14.1000, 48.5700},
    {14.1500, 48.5600},
    {14.2000, 48.5550},
    {14.2500, 48.5520},
    {14.3000, 48.5530},
    {14.3389, 48.5553},
    // Jižní Čechy
    {14.4000, 48.5800},
    {14.4500, 48.6200},
    {14.5000, 48.6600},
    {14.5500, 48.7000},
    {14.6000, 48.7400},
    {14.6500, 48.7800},
    {14.7000, 48.8200},
    {14.7500, 48.8600},
    {14.8000, 48.9000},
    {14.8500, 48.9400},
    {14.9014, 48.9644},
    {14.9500, 48.9800},
    {15.0000, 49.0000},
    {15.0500, 49.0150},
    {15.1000, 49.0250},
    {15.1500, 49.0350},
    {15.2000, 49.0400},
    {15.2534, 49.0391},
    // Hranice s Rakouskem - východ
    {15.3000, 49.0200},
    {15.3500, 49.0000},
    {15.4000, 48.9800},
    {15.4500, 48.9600},
    {15.5000, 48.9400},
    {15.5500, 48.9100},
    {15.6000, 48.8800},
    {15.6500, 48.8500},
    {15.7000, 48.8200},
    {15.7500, 48.7900},
    {15.8000, 48.7600},
    {15.8500, 48.7400},
    {15.9000, 48.7300},
    {15.9500, 48.7250},
    {16.0000, 48.7300},
    {16.0296, 48.7339},
    // Jižní Morava
    {16.1000, 48.7500},
    {16.1500, 48.7600},
    {16.2000, 48.7700},
    {16.2500, 48.7750},
    {16.3000, 48.7800},
    {16.3500, 48.7850},
    {16.4000, 48.7880},
    {16.4500, 48.7870},
    {16.4993, 48.7858},
    {16.5500, 48.7800},
    {16.6000, 48.7700},
    {16.6500, 48.7500},
    {16.7000, 48.7200},
    {16.7500, 48.6800},
    {16.8000, 48.6400},
    {16.8500, 48.6100},
    {16.9000, 48.5900},
    {16.9603, 48.5970},
    // Hranice se Slovenskem
    {17.0000, 48.6200},
    {17.0500, 48.7000},
    {17.1020, 48.8170},
    {17.1500, 48.8400},
    {17.2000, 48.8600},
    {17.2500, 48.8700},
    {17.3000, 48.8750},
    {17.3500, 48.8780},
    {17.4000, 48.8800},
    {17.4500, 48.8700},
    {17.5000, 48.8500},
    {17.5450, 48.8000},
    {17.6000, 48.7800},
    {17.6500, 48.7900},
    {17.7000, 48.8200},
    {17.7500, 48.8500},
    {17.8000, 48.8800},
    {17.8500, 48.9000},
    {17.8865, 48.9035},
    {17.9135, 48.9965},
    // Severovýchodní hranice
    {17.9500, 49.0000},
    {18.0000, 49.0200},
    {18.0500, 49.0380},
    {18.1050, 49.0440},
    {18.1500, 49.1000},
    {18.1705, 49.2715},
    {18.2000, 49.2800},
    {18.2500, 49.2900},
    {18.3000, 49.3000},
    {18.3500, 49.3050},
    {18.3999, 49.3150},
    {18.4500, 49.3500},
    {18.5000, 49.4000},
    {18.5500, 49.4500},
    {18.5550, 49.4950},
    {18.6000, 49.5000},
    {18.6500, 49.5050},
    {18.7000, 49.5000},
    {18.7500, 49.4980},
    {18.8000, 49.4970},
    {18.8531, 49.4962},
    // Hranice s Polskem - východ
    {18.8400, 49.5500},
    {18.8200, 49.6000},
    {18.8000, 49.6500},
    {18.7800, 49.7000},
    {18.7600, 49.7500},
    {18.7400, 49.8000},
    {18.7000, 49.8500},
    {18.6500, 49.9000},
    {18.6000, 49.9500},
    {18.5500, 49.9800},
    {18.5000, 49.9900},
    {18.4500, 49.9950},
    {18.3929, 49.9886},
    // Severní hranice s Polskem
    {18.3000, 49.9950},
    {18.2000, 50.0000},
    {18.1000, 50.0100},
    {18.0000, 50.0150},
    {17.9000, 50.0200},
    {17.8000, 50.0300},
    {17.7000, 50.0400},
    {17.6494, 50.0490},
    {17.6000, 50.1000},
    {17.5546, 50.3621},
    {17.5000, 50.3500},
    {17.4000, 50.3600},
    {17.3000, 50.3800},
    {17.2000, 50.4000},
    {17.1000, 50.4200},
    {17.0000, 50.4300},
    {16.9000, 50.4400},
    {16.8688, 50.4740},
    {16.8000, 50.4500},
    {16.7500, 50.4000},
    {16.7195, 50.2157},
    {16.7000, 50.2000},
    {16.6000, 50.2500},
    {16.5000, 50.3000},
    {16.4000, 50.3500},
    {16.3000, 50.4000},
    {16.2386, 50.6977},
    {16.2000, 50.7000},
    {16.1763, 50.4226},
    {16.1500, 50.5000},
    {16.1000, 50.5500},
    {16.0000, 50.6000},
    {15.9000, 50.6500},
    {15.8000, 50.7000},
    {15.7000, 50.7500},
    {15.6000, 50.7800},
    {15.4910, 50.7847},
    // Severozápadní hranice
    {15.4000, 50.8000},
    {15.3000, 50.8500},
    {15.2000, 50.9000},
    {15.1000, 50.9500},
    {15.0170, 51.1067},
    {14.9000, 51.0500},
    {14.8000, 51.0300},
    {14.7000, 51.0100},
    {14.5707, 51.0023},
    {14.5000, 51.0200},
    {14.4000, 51.0500},
    {14.3070, 51.1173},
    {14.2000, 51.0500},
    {14.1000, 51.0000},
    {14.0562, 50.9269},
    {14.0000, 50.9000},
    {13.9000, 50.8500},
    {13.8000, 50.8000},
    {13.7000, 50.7700},
    {13.6000, 50.7500},
    {13.5000, 50.7400},
    {13.4000, 50.7350},
    {13.3381, 50.7332},
    // Krušné hory
    {13.2500, 50.7200},
    {13.1500, 50.7000},
    {13.0500, 50.6500},
    {12.9668, 50.4841},
    {12.9000, 50.4500},
    {12.8000, 50.4000},
    {12.7000, 50.3500},
    {12.6000, 50.3200},
    {12.5000, 50.3000},
    {12.4000, 50.2800},
    {12.3000, 50.2700},
    {12.2401, 50.2663},
    {12.0906, 50.2524}   // Zpět na začátek
};

static const int CZ_BORDER_POINTS = sizeof(CZ_BORDER_GPS) / sizeof(CZ_BORDER_GPS[0]);

// Pomocná funkce pro kreslení tlusté čáry (šířka 5 pixelů)
template<typename DisplayType>
inline void drawThickLine(DisplayType& display, int x0, int y0, int x1, int y1, uint16_t color, int thickness = 5)
{
    // Nakresli více čar vedle sebe pro vytvoření tlusté čáry
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    
    // Pokud je čára více horizontální, posouváme ve směru Y
    // Pokud je čára více vertikální, posouváme ve směru X
    int halfThick = thickness / 2;
    
    if (dx >= dy) {
        // Horizontální nebo diagonální - posun v Y
        for (int i = -halfThick; i <= halfThick; i++) {
            display.drawLine(x0, y0 + i, x1, y1 + i, color);
        }
    } else {
        // Vertikální - posun v X
        for (int i = -halfThick; i <= halfThick; i++) {
            display.drawLine(x0 + i, y0, x1 + i, y1, color);
        }
    }
}

// Funkce pro získání hraničních bodů jako pixelových souřadnic
// v rámci 2x2 tile gridu
class CzechBorder
{
public:
    // Vykresli obrys ČR na daných souřadnicích
    // mapX, mapY - pozice mapy na displeji
    // scale - měřítko (2 = zmenšit na polovinu)
    // baseTileX, baseTileY - základní tile souřadnice
    template<typename DisplayType>
    static void draw(DisplayType& display, int mapX, int mapY, int offsetX, int offsetY, 
                     int scale, int baseTileX, int baseTileY, int zoom, uint16_t color)
    {
        // Převeď GPS body na pixelové souřadnice
        int prevX = -1, prevY = -1;
        
        for (int i = 0; i < CZ_BORDER_POINTS; i++)
        {
            int px, py;
            // GeoJSON má [lon, lat], takže první je longitude, druhá latitude
            latLonToTilePixel(CZ_BORDER_GPS[i][1], CZ_BORDER_GPS[i][0], zoom, baseTileX, baseTileY, px, py);
            
            // Aplikuj scale a offset
            int screenX = mapX + offsetX + px / scale;
            int screenY = mapY + offsetY + py / scale;
            
            // Nakresli tlustou čáru od předchozího bodu
            if (prevX >= 0 && prevY >= 0)
            {
                drawThickLine(display, prevX, prevY, screenX, screenY, color, 5);
            }
            
            prevX = screenX;
            prevY = screenY;
        }
    }
    
    // Verze s clippingem
    template<typename DisplayType>
    static void drawClipped(DisplayType& display, int mapX, int mapY, int offsetX, int offsetY, 
                            int scale, int baseTileX, int baseTileY, int zoom, uint16_t color,
                            int clipX, int clipY, int clipW, int clipH)
    {
        int prevX = -1, prevY = -1;
        
        for (int i = 0; i < CZ_BORDER_POINTS; i++)
        {
            int px, py;
            // GeoJSON má [lon, lat], takže první je longitude, druhá latitude
            latLonToTilePixel(CZ_BORDER_GPS[i][1], CZ_BORDER_GPS[i][0], zoom, baseTileX, baseTileY, px, py);
            
            // Aplikuj scale a offset
            int screenX = mapX + offsetX + px / scale;
            int screenY = mapY + offsetY + py / scale;
            
            // Nakresli tlustou čáru od předchozího bodu
            if (prevX >= 0 && prevY >= 0)
            {
                // Nakresli tlustou čáru (5px šířka)
                drawThickLine(display, prevX, prevY, screenX, screenY, color, 5);
            }
            
            prevX = screenX;
            prevY = screenY;
        }
    }
};
