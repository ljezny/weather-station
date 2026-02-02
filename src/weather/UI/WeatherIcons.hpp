#pragma once
#include <Arduino.h>
#include "Display.hpp"
#include "../API/OpenMeteoAPI.hpp"

// Weather icons for e-ink display (32x32 pixels)
// Icons are drawn procedurally for e-ink compatibility

class WeatherIcons
{
public:
    // Draw weather icon at specified position (centered)
    static void drawIcon(Display& display, WeatherIconType type, int centerX, int centerY, int size = 32, bool isDay = true)
    {
        int halfSize = size / 2;
        int x = centerX - halfSize;
        int y = centerY - halfSize;
        
        switch (type)
        {
            case ICON_SUN:
                drawSun(display, centerX, centerY, halfSize);
                break;
            
            case ICON_PARTLY_CLOUDY:
                drawPartlyCloudy(display, centerX, centerY, halfSize, isDay);
                break;
            
            case ICON_CLOUDY:
                drawCloud(display, centerX, centerY, halfSize);
                break;
            
            case ICON_FOG:
                drawFog(display, centerX, centerY, halfSize);
                break;
            
            case ICON_DRIZZLE:
                drawDrizzle(display, centerX, centerY, halfSize);
                break;
            
            case ICON_RAIN:
                drawRain(display, centerX, centerY, halfSize);
                break;
            
            case ICON_HEAVY_RAIN:
                drawHeavyRain(display, centerX, centerY, halfSize);
                break;
            
            case ICON_SNOW:
                drawSnow(display, centerX, centerY, halfSize);
                break;
            
            case ICON_THUNDERSTORM:
                drawThunderstorm(display, centerX, centerY, halfSize);
                break;
            
            default:
                drawUnknown(display, centerX, centerY, halfSize);
                break;
        }
    }
    
    // Draw icon based on weather code
    static void drawIconForCode(Display& display, int weatherCode, int centerX, int centerY, int size = 32, bool isDay = true)
    {
        WeatherIconType type = OpenMeteoAPI::getIconType(weatherCode, isDay);
        drawIcon(display, type, centerX, centerY, size, isDay);
    }

private:
    // Sun icon
    static void drawSun(Display& display, int cx, int cy, int r)
    {
        // Center circle
        int innerR = r * 2 / 3;
        display.fillCircle(cx, cy, innerR, COLOR_BLACK);
        display.fillCircle(cx, cy, innerR - 2, COLOR_WHITE);
        
        // Rays
        int rayLen = r - 2;
        int rayStart = innerR + 2;
        for (int i = 0; i < 8; i++)
        {
            float angle = i * PI / 4;
            int x1 = cx + rayStart * cos(angle);
            int y1 = cy + rayStart * sin(angle);
            int x2 = cx + rayLen * cos(angle);
            int y2 = cy + rayLen * sin(angle);
            display.drawLine(x1, y1, x2, y2, COLOR_BLACK);
        }
    }
    
    // Cloud icon
    static void drawCloud(Display& display, int cx, int cy, int r)
    {
        int cloudY = cy;
        int smallR = r / 2;
        int bigR = r * 2 / 3;
        
        // Main cloud body (three overlapping circles)
        display.fillCircle(cx - smallR, cloudY, smallR, COLOR_BLACK);
        display.fillCircle(cx, cloudY - smallR / 2, bigR, COLOR_BLACK);
        display.fillCircle(cx + smallR, cloudY, smallR, COLOR_BLACK);
        
        // White inner fill
        display.fillCircle(cx - smallR, cloudY, smallR - 2, COLOR_WHITE);
        display.fillCircle(cx, cloudY - smallR / 2, bigR - 2, COLOR_WHITE);
        display.fillCircle(cx + smallR, cloudY, smallR - 2, COLOR_WHITE);
        
        // Bottom flat edge
        display.fillRect(cx - r + 3, cloudY, r * 2 - 6, smallR, COLOR_WHITE);
        display.drawLine(cx - r + 3, cloudY + smallR - 2, cx + r - 3, cloudY + smallR - 2, COLOR_BLACK);
    }
    
    // Partly cloudy (sun + small cloud)
    static void drawPartlyCloudy(Display& display, int cx, int cy, int r, bool isDay)
    {
        // Draw smaller sun in top-left
        if (isDay)
        {
            drawSun(display, cx - r / 3, cy - r / 3, r / 2);
        }
        else
        {
            // Moon at night
            display.fillCircle(cx - r / 3, cy - r / 3, r / 2, COLOR_BLACK);
            display.fillCircle(cx - r / 3 + r / 4, cy - r / 3 - r / 6, r / 2, COLOR_WHITE);
        }
        
        // Draw cloud in front (bottom-right)
        int cloudX = cx + r / 4;
        int cloudY = cy + r / 4;
        int cloudR = r * 2 / 3;
        
        // Cloud with fill
        int smallR = cloudR / 2;
        display.fillCircle(cloudX - smallR / 2, cloudY, smallR, COLOR_BLACK);
        display.fillCircle(cloudX + smallR / 4, cloudY - smallR / 3, smallR, COLOR_BLACK);
        display.fillCircle(cloudX + smallR, cloudY, smallR * 3 / 4, COLOR_BLACK);
        display.fillCircle(cloudX - smallR / 2, cloudY, smallR - 2, COLOR_WHITE);
        display.fillCircle(cloudX + smallR / 4, cloudY - smallR / 3, smallR - 2, COLOR_WHITE);
        display.fillCircle(cloudX + smallR, cloudY, smallR * 3 / 4 - 2, COLOR_WHITE);
        display.fillRect(cloudX - smallR + 2, cloudY, smallR * 2 + 2, smallR - 2, COLOR_WHITE);
    }
    
    // Fog icon
    static void drawFog(Display& display, int cx, int cy, int r)
    {
        int lineSpacing = r / 3;
        int lineWidth = r * 3 / 2;
        
        for (int i = -1; i <= 1; i++)
        {
            int y = cy + i * lineSpacing;
            display.drawLine(cx - lineWidth / 2, y, cx + lineWidth / 2, y, COLOR_BLACK);
            display.drawLine(cx - lineWidth / 2, y + 1, cx + lineWidth / 2, y + 1, COLOR_BLACK);
        }
    }
    
    // Drizzle icon (cloud + light drops)
    static void drawDrizzle(Display& display, int cx, int cy, int r)
    {
        // Small cloud at top
        int cloudY = cy - r / 3;
        drawSmallCloud(display, cx, cloudY, r * 2 / 3);
        
        // Light rain drops (dots)
        int dropY = cy + r / 3;
        for (int i = -1; i <= 1; i++)
        {
            display.fillCircle(cx + i * (r / 2), dropY, 2, COLOR_BLACK);
        }
    }
    
    // Rain icon
    static void drawRain(Display& display, int cx, int cy, int r)
    {
        // Cloud at top
        int cloudY = cy - r / 3;
        drawSmallCloud(display, cx, cloudY, r * 2 / 3);
        
        // Rain lines
        int dropStartY = cy;
        int dropLen = r / 2;
        for (int i = -1; i <= 1; i++)
        {
            int x = cx + i * (r / 2);
            display.drawLine(x, dropStartY, x - 2, dropStartY + dropLen, COLOR_BLACK);
            display.drawLine(x + 1, dropStartY, x - 1, dropStartY + dropLen, COLOR_BLACK);
        }
    }
    
    // Heavy rain icon
    static void drawHeavyRain(Display& display, int cx, int cy, int r)
    {
        // Cloud at top
        int cloudY = cy - r / 2;
        drawSmallCloud(display, cx, cloudY, r * 2 / 3);
        
        // Heavy rain lines (more and longer)
        int dropStartY = cy - r / 6;
        int dropLen = r * 2 / 3;
        for (int i = -2; i <= 2; i++)
        {
            int x = cx + i * (r / 3);
            display.drawLine(x, dropStartY, x - 3, dropStartY + dropLen, COLOR_BLACK);
            display.drawLine(x + 1, dropStartY, x - 2, dropStartY + dropLen, COLOR_BLACK);
        }
    }
    
    // Snow icon
    static void drawSnow(Display& display, int cx, int cy, int r)
    {
        // Cloud at top
        int cloudY = cy - r / 3;
        drawSmallCloud(display, cx, cloudY, r * 2 / 3);
        
        // Snowflakes (asterisks)
        int flakeY = cy + r / 3;
        for (int i = -1; i <= 1; i++)
        {
            int fx = cx + i * (r / 2);
            int fy = flakeY + ((i + 1) % 2) * (r / 5);
            drawSnowflake(display, fx, fy, r / 6);
        }
    }
    
    // Thunderstorm icon
    static void drawThunderstorm(Display& display, int cx, int cy, int r)
    {
        // Dark cloud at top
        int cloudY = cy - r / 2;
        int smallR = r / 3;
        display.fillCircle(cx - smallR / 2, cloudY, smallR, COLOR_BLACK);
        display.fillCircle(cx + smallR / 4, cloudY - smallR / 3, smallR, COLOR_BLACK);
        display.fillCircle(cx + smallR, cloudY, smallR * 3 / 4, COLOR_BLACK);
        
        // Lightning bolt
        int boltX = cx;
        int boltY = cy + r / 6;
        int boltH = r * 2 / 3;
        
        // Lightning shape
        display.drawLine(boltX, boltY - boltH / 2, boltX - 3, boltY, COLOR_RED);
        display.drawLine(boltX - 3, boltY, boltX + 3, boltY, COLOR_RED);
        display.drawLine(boltX + 3, boltY, boltX, boltY + boltH / 2, COLOR_RED);
        
        // Thicker bolt
        display.drawLine(boltX + 1, boltY - boltH / 2, boltX - 2, boltY, COLOR_RED);
        display.drawLine(boltX + 4, boltY, boltX + 1, boltY + boltH / 2, COLOR_RED);
    }
    
    // Unknown weather icon
    static void drawUnknown(Display& display, int cx, int cy, int r)
    {
        display.drawCircle(cx, cy, r - 2, COLOR_BLACK);
        display.drawText(MEDIUM, "?", cx, cy, CENTER, CENTER, 0, 0, COLOR_BLACK);
    }
    
    // Helper: Small filled cloud
    static void drawSmallCloud(Display& display, int cx, int cy, int r)
    {
        int smallR = r / 2;
        display.fillCircle(cx - smallR / 2, cy, smallR, COLOR_BLACK);
        display.fillCircle(cx + smallR / 4, cy - smallR / 3, smallR, COLOR_BLACK);
        display.fillCircle(cx + smallR, cy, smallR * 3 / 4, COLOR_BLACK);
        display.fillCircle(cx - smallR / 2, cy, smallR - 2, COLOR_WHITE);
        display.fillCircle(cx + smallR / 4, cy - smallR / 3, smallR - 2, COLOR_WHITE);
        display.fillCircle(cx + smallR, cy, smallR * 3 / 4 - 2, COLOR_WHITE);
        display.fillRect(cx - smallR + 2, cy, smallR * 2, smallR - 2, COLOR_WHITE);
    }
    
    // Helper: Draw snowflake
    static void drawSnowflake(Display& display, int cx, int cy, int r)
    {
        // 6-pointed star
        for (int i = 0; i < 3; i++)
        {
            float angle = i * PI / 3;
            int x1 = cx + r * cos(angle);
            int y1 = cy + r * sin(angle);
            int x2 = cx - r * cos(angle);
            int y2 = cy - r * sin(angle);
            display.drawLine(x1, y1, x2, y2, COLOR_BLACK);
        }
        display.drawPixel(cx, cy, COLOR_BLACK);
    }
};
