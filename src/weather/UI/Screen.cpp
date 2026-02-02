#include "Screen.hpp"
#include "WeatherIcons.hpp"
#include "gfxlatin2.h"
#include "decodeutf8.h"
#include "../Localization/Localization.hpp"
#include "../consts.h"
#include <qrcode.h>

WeatherScreen::WeatherScreen() : display()
{
}

void WeatherScreen::drawBootScreen()
{
    display.beginDraw();
    display.firstPage();
    do {
        display.clear();
        
        int centerX = display.getDisplayWidth() / 2;
        int centerY = display.getDisplayHeight() / 2;
        
        // Draw logo/title
        display.drawText(HUGE, "Weather", centerX, centerY - 50,
            CENTER, CENTER, 0, 0, GxEPD_BLACK);
        display.drawText(LARGE, "Station", centerX, centerY + 20,
            CENTER, CENTER, 0, 0, GxEPD_DARKGREY);
        
        // Version
        String versionStr = "v" + String(VERSION_NUMBER);
        display.drawText(SMALL, versionStr, centerX, display.getDisplayHeight() - 40,
            CENTER, CENTER, 0, 0, GxEPD_DARKGREY);
            
    } while (display.nextPage());
    display.endDraw();
}

void WeatherScreen::drawPopup(ScreenInfoData_t info)
{
    // Define popup dimensions - at bottom of screen, aligned to 8 pixels
    int popupHeight = 400;  // divisible by 8
    int popupWidth = display.getDisplayWidth();  // full width, divisible by 8
    int popupX = 0;
    int popupY = display.getDisplayHeight() - popupHeight;  // align to bottom
    
    display.setPopupMode(true, popupX, popupY, popupWidth, popupHeight);
    display.beginDraw();
    
    // White background with border
    display.fillRect(popupX, popupY, popupWidth, popupHeight, GxEPD_WHITE);
    display.fillRect(popupX, popupY, popupWidth, 4, GxEPD_BLACK);  // Top border
    
    int contentY = popupY + 40;
    int centerX = display.getDisplayWidth() / 2;
    
    // Title
    if (info.title.length() > 0) {
        display.drawText(MEDIUM, info.title, centerX, contentY,
            CENTER, LEADING, 0, 0, GxEPD_BLACK);
        contentY += 50;
    }
    
    // QR Code
    if (info.qrCode.length() > 0) {
        // Generate QR code
        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(3)];
        qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, info.qrCode.c_str());
        
        int qrSize = qrcode.size;
        int scale = min((popupHeight - 120) / qrSize, 6);  // Scale QR to fit
        int qrDrawSize = qrSize * scale;
        int qrX = centerX - qrDrawSize / 2;
        int qrY = contentY;
        
        for (int y = 0; y < qrSize; y++) {
            for (int x = 0; x < qrSize; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    display.fillRect(qrX + x * scale, qrY + y * scale, scale, scale, GxEPD_BLACK);
                }
            }
        }
        contentY += qrDrawSize + 20;
    }
    
    // Subtitle
    if (info.subtitle.length() > 0) {
        display.drawText(SMALL, info.subtitle, centerX, contentY,
            CENTER, LEADING, 0, 0, GxEPD_DARKGREY);
    }
    
    // Temperature (if shown)
    if (info.showTemperature) {
        String tempStr = String(info.temperature, 1) + "°C";
        display.drawText(SMALL, tempStr, popupX + popupWidth - 20, popupY + popupHeight - 30,
            TRAILING, TRAILING, 0, 0, GxEPD_DARKGREY);
    }
    
    // Use windowed partial refresh - only updates popup area
    display.updateWindow(popupX, popupY, popupWidth, popupHeight);
    display.endDraw();
    display.setPopupMode(false);
}

void WeatherScreen::drawWebSetupPopupOverlay(String softAPSSID, String softAPIP, String staSSID, String staIP)
{
    // Draw popup at bottom, aligned to 8 pixels
    int popupHeight = 200;  // divisible by 8: 200 = 25*8
    popupHeight = (popupHeight / 8) * 8;  // ensure divisible by 8
    int popupWidth = display.getDisplayWidth();  // full width
    int popupX = 0;
    int popupY = display.getDisplayHeight() - popupHeight;
    
    display.setPopupMode(true, popupX, popupY, popupWidth, popupHeight);
    display.beginDraw();
    
    display.fillRect(popupX, popupY, popupWidth, popupHeight, GxEPD_WHITE);
    display.fillRect(popupX, popupY, popupWidth, 3, GxEPD_BLACK);  // Top border
    
    int textX = popupX + 20;
    int textY = popupY + 30;
    
    display.drawText(MEDIUM, Localization::get(STR_STATUS_STARTING), textX, textY,
        LEADING, LEADING, 0, 0, GxEPD_BLACK);
    textY += 35;
    
    // Show AP info
    display.drawText(TINY, "WiFi: " + softAPSSID, textX, textY,
        LEADING, LEADING, 0, 0, GxEPD_DARKGREY);
    textY += 25;
    
    display.drawText(TINY, "IP: " + softAPIP, textX, textY,
        LEADING, LEADING, 0, 0, GxEPD_DARKGREY);
    textY += 35;
    
    // Show STA info if connected
    if (staIP != "0.0.0.0") {
        display.drawText(TINY, String(Localization::get(STR_STATUS_CONNECTED_TO)) + ": " + staSSID, textX, textY,
            LEADING, LEADING, 0, 0, GxEPD_DARKGREY);
        textY += 25;
        display.drawText(TINY, "IP: " + staIP, textX, textY,
            LEADING, LEADING, 0, 0, GxEPD_DARKGREY);
    }
    
    // Use windowed partial refresh
    display.updateWindow(popupX, popupY, popupWidth, popupHeight);
    display.endDraw();
    display.setPopupMode(false);
}

void WeatherScreen::drawWeatherScreen(WeatherScreenData_t& screenData, WeatherData_t& weatherData)
{
    display.beginDraw();
    display.firstPage();
    do {
        display.clear();
        
        int screenW = display.getDisplayWidth();
        int screenH = display.getDisplayHeight();
        
        int y = 0;
        
        // ========== Header ==========
        // "Weather Dashboard" + Location + Date
        int headerHeight = 90;
        drawHeader(0, y, screenW, screenData.locationName.c_str(), screenData.currentTime);
        y += headerHeight;
        
        drawHorizontalLine(MARGIN, y, screenW - 2 * MARGIN);
        y += 10;
        
        // ========== Current Weather ==========
        // Large icon + temperature + details (wind, humidity, pressure)
        int currentHeight = 200;
        drawCurrentWeather(0, y, screenW, currentHeight, weatherData.current);
        y += currentHeight;
        
        drawHorizontalLine(MARGIN, y, screenW - 2 * MARGIN);
        y += 10;
        
        // ========== Today's Forecast ==========
        // Morning, Afternoon, Evening, Night
        int todayHeight = 180;
        drawTodayForecast(0, y, screenW, todayHeight, weatherData.hourly, weatherData.hourlyCount, screenData.currentTime);
        y += todayHeight;
        
        drawHorizontalLine(MARGIN, y, screenW - 2 * MARGIN);
        y += 10;
        
        // ========== 3-Day Outlook ==========
        int outlookHeight = 180;
        drawDailyOutlook(0, y, screenW, outlookHeight, weatherData.daily, weatherData.dailyCount);
        y += outlookHeight;
        
        drawHorizontalLine(MARGIN, y, screenW - 2 * MARGIN);
        y += 10;
        
        // ========== Footer ==========
        // UV Index + Sunrise/Sunset
        if (weatherData.dailyCount > 0) {
            // UV index is estimated from weather code (not available in API)
            int uvIndex = (weatherData.current.isDay && weatherData.current.weatherCode <= 3) ? 5 : 2;
            drawFooter(0, y, screenW, weatherData.daily[0], uvIndex);
        }
        
    } while (display.nextPage());
    display.endDraw();
}

// ============================================================================
// Header Section
// ============================================================================

void WeatherScreen::drawHeader(int x, int y, int width, const char* locationName, time_t currentTime)
{
    int centerX = x + width / 2;
    
    // Title: "Weather Dashboard"
    display.drawText(LARGE, Localization::get(STR_LABEL_WEATHER_DASHBOARD), centerX, y + 10,
        CENTER, LEADING, 0, 0, GxEPD_BLACK);
    
    // Location
    display.drawText(SMALL, locationName, centerX, y + 45,
        CENTER, LEADING, 0, 0, GxEPD_DARKGREY);
    
    // Date (e.g., "Tuesday, 24 Jan")
    struct tm timeinfo;
    localtime_r(&currentTime, &timeinfo);
    
    String dateStr = String(Localization::getDayFull(timeinfo.tm_wday)) + ", " + 
                     String(timeinfo.tm_mday) + " " + 
                     Localization::getMonthShort(timeinfo.tm_mon);
    
    display.drawText(SMALL, dateStr, centerX, y + 70,
        CENTER, LEADING, 0, 0, GxEPD_DARKGREY);
}

// ============================================================================
// Current Weather Section
// ============================================================================

void WeatherScreen::drawCurrentWeather(int x, int y, int width, int height, CurrentWeather_t& current)
{
    // Determine if day/night
    bool isDay = current.isDay;
    
    // Left side: Large weather icon
    int iconSize = 120;
    int iconX = x + MARGIN + 30;
    int iconY = y + (height - iconSize) / 2;
    drawWeatherIcon(iconX, iconY, iconSize, current.weatherCode, isDay);
    
    // Center: Large temperature
    int tempX = x + width / 2 - 20;
    int tempY = y + 30;
    String tempStr = String((int)round(current.temperature)) + "°";
    display.drawText(HUGE, tempStr, tempX, tempY, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Right of temperature: Weather description and feels like
    int descX = tempX + 100;
    String weatherDesc = Localization::get((StringID)(STR_WEATHER_CLEAR + min(current.weatherCode, 13)));
    display.drawText(SMALL, weatherDesc, descX, tempY + 10, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    String feelsLike = String(Localization::get(STR_WEATHER_FEELS_LIKE)) + ": " + 
                       String((int)round(current.apparentTemperature)) + "°";
    display.drawText(TINY, feelsLike, descX, tempY + 35, LEADING, LEADING, 0, 0, GxEPD_DARKGREY);
    
    // Details on the right side
    int detailsX = x + width - MARGIN - 150;
    int detailsY = y + 50;
    int detailSpacing = 35;
    
    // Wind
    String windStr = "• " + String(Localization::get(STR_LABEL_WIND)) + ": " + 
                     String((int)current.windSpeed) + " km/h";
    display.drawText(TINY, windStr, detailsX, detailsY, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    detailsY += detailSpacing;
    
    // Humidity
    String humidityStr = "• " + String(Localization::get(STR_LABEL_HUMIDITY)) + ": " + 
                         String(current.humidity) + "%";
    display.drawText(TINY, humidityStr, detailsX, detailsY, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    detailsY += detailSpacing;
    
    // Precipitation
    String precipStr = "• " + String(Localization::get(STR_LABEL_PRECIPITATION)) + ": " + 
                       String(current.precipitation, 1) + " mm";
    display.drawText(TINY, precipStr, detailsX, detailsY, LEADING, LEADING, 0, 0, GxEPD_BLACK);
}

// ============================================================================
// Today's Forecast Section (4 time periods)
// ============================================================================

void WeatherScreen::drawTodayForecast(int x, int y, int width, int height, HourlyForecast_t* hourly, int count, time_t currentTime)
{
    // Title
    display.drawText(MEDIUM, Localization::get(STR_LABEL_TODAYS_FORECAST), x + MARGIN, y + 5,
        LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Find relevant hours: Morning (6-12), Afternoon (12-18), Evening (18-22), Night (22-6)
    // We'll pick representative hours: 9, 14, 19, 23
    int targetHours[] = {9, 14, 19, 23};
    int periodCount = 4;
    
    struct tm currentTm;
    localtime_r(&currentTime, &currentTm);
    int currentHour = currentTm.tm_hour;
    
    int columnWidth = (width - 2 * MARGIN) / periodCount;
    int contentY = y + 40;
    int iconSize = 50;
    
    for (int p = 0; p < periodCount; p++) {
        int targetHour = targetHours[p];
        
        // Find the hourly forecast closest to this target
        int bestIdx = -1;
        int bestDiff = 999;
        for (int i = 0; i < min(count, 24); i++) {
            int diff = abs(hourly[i].hour - targetHour);
            if (diff < bestDiff) {
                bestDiff = diff;
                bestIdx = i;
            }
        }
        
        int colX = x + MARGIN + p * columnWidth + columnWidth / 2;
        
        // Period name
        const char* periodName = getTimePeriodName(targetHour);
        display.drawText(TINY, periodName, colX, contentY, CENTER, LEADING, 0, 0, GxEPD_BLACK);
        
        if (bestIdx >= 0) {
            HourlyForecast_t& h = hourly[bestIdx];
            
            // Icon
            bool isDay = (targetHour >= 6 && targetHour < 20);
            drawWeatherIcon(colX - iconSize/2, contentY + 20, iconSize, h.weatherCode, isDay);
            
            // Temperature
            String tempStr = String((int)round(h.temperature)) + "°";
            display.drawText(MEDIUM, tempStr, colX, contentY + 75, CENTER, LEADING, 0, 0, GxEPD_BLACK);
            
            // Description
            String desc = Localization::get((StringID)(STR_WEATHER_CLEAR + min(h.weatherCode, 13)));
            display.drawText(EXTRA_SMALL, desc, colX, contentY + 105, CENTER, LEADING, 0, 0, GxEPD_DARKGREY);
        }
    }
}

// ============================================================================
// 3-Day Outlook Section
// ============================================================================

void WeatherScreen::drawDailyOutlook(int x, int y, int width, int height, DailyForecast_t* daily, int count)
{
    // Title
    display.drawText(MEDIUM, Localization::get(STR_LABEL_3DAY_OUTLOOK), x + MARGIN, y + 5,
        LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Show next 3 days (skip today = index 0)
    int daysToShow = min(count - 1, 3);
    if (daysToShow <= 0) return;
    
    int columnWidth = (width - 2 * MARGIN) / daysToShow;
    int contentY = y + 40;
    int iconSize = 50;
    
    for (int d = 0; d < daysToShow; d++) {
        DailyForecast_t& day = daily[d + 1];  // Skip today
        
        int colX = x + MARGIN + d * columnWidth + columnWidth / 2;
        
        // Day name
        display.drawText(SMALL, Localization::getDayShort(day.dayOfWeek), colX, contentY, 
            CENTER, LEADING, 0, 0, GxEPD_BLACK);
        
        // Icon
        drawWeatherIcon(colX - iconSize/2, contentY + 25, iconSize, day.weatherCode, true);
        
        // Description
        String desc = Localization::get((StringID)(STR_WEATHER_CLEAR + min(day.weatherCode, 13)));
        display.drawText(EXTRA_SMALL, desc, colX, contentY + 80, CENTER, LEADING, 0, 0, GxEPD_DARKGREY);
        
        // High/Low temps
        String highStr = String(Localization::get(STR_LABEL_HIGH)) + ": " + String((int)round(day.tempMax)) + "°";
        String lowStr = String(Localization::get(STR_LABEL_LOW)) + ": " + String((int)round(day.tempMin)) + "°";
        display.drawText(TINY, highStr, colX, contentY + 105, CENTER, LEADING, 0, 0, GxEPD_BLACK);
        display.drawText(TINY, lowStr, colX, contentY + 125, CENTER, LEADING, 0, 0, GxEPD_DARKGREY);
    }
}

// ============================================================================
// Footer Section (UV Index + Sunrise/Sunset)
// ============================================================================

void WeatherScreen::drawFooter(int x, int y, int width, DailyForecast_t& today, int uvIndex)
{
    int halfWidth = width / 2;
    
    // Left side: UV Index
    int uvX = x + MARGIN;
    display.drawText(SMALL, Localization::get(STR_LABEL_UV_INDEX), uvX, y + 5,
        LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // UV level bar
    int barX = uvX;
    int barY = y + 35;
    int barWidth = 150;
    int barHeight = 20;
    
    // Draw UV bar background
    display.fillRect(barX, barY, barWidth, barHeight, GxEPD_LIGHTGREY);
    
    // UV level indicator (0-11+ scale)
    int uvLevel = min(uvIndex, 11);
    int indicatorWidth = (barWidth * uvLevel) / 11;
    display.fillRect(barX, barY, indicatorWidth, barHeight, GxEPD_DARKGREY);
    
    // UV level text
    const char* uvText;
    if (uvIndex <= 2) uvText = Localization::get(STR_UV_LOW);
    else if (uvIndex <= 5) uvText = Localization::get(STR_UV_MODERATE);
    else if (uvIndex <= 7) uvText = Localization::get(STR_UV_HIGH);
    else if (uvIndex <= 10) uvText = Localization::get(STR_UV_VERY_HIGH);
    else uvText = Localization::get(STR_UV_EXTREME);
    
    display.drawText(TINY, uvText, barX + barWidth + 10, barY + 3,
        LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Right side: Sunrise & Sunset
    int sunX = x + halfWidth + 30;
    display.drawText(SMALL, Localization::get(STR_LABEL_SUNRISE_SUNSET), sunX, y + 5,
        LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Sunrise icon (simple sun rising)
    drawWeatherIcon(sunX, y + 30, 40, 0, true);  // Sun icon
    
    // Format sunrise/sunset times (remove date prefix if present)
    String sunriseTime = today.sunrise;
    String sunsetTime = today.sunset;
    
    // Extract time part (format: "2024-01-15T07:30")
    int tIdx = sunriseTime.indexOf('T');
    if (tIdx > 0) {
        sunriseTime = sunriseTime.substring(tIdx + 1, tIdx + 6);
    }
    tIdx = sunsetTime.indexOf('T');
    if (tIdx > 0) {
        sunsetTime = sunsetTime.substring(tIdx + 1, tIdx + 6);
    }
    
    String sunriseStr = String(Localization::get(STR_LABEL_SUNRISE)) + ": " + sunriseTime;
    display.drawText(TINY, sunriseStr, sunX + 50, y + 35, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    String sunsetStr = String(Localization::get(STR_LABEL_SUNSET)) + ": " + sunsetTime;
    display.drawText(TINY, sunsetStr, sunX + 50, y + 55, LEADING, LEADING, 0, 0, GxEPD_BLACK);
}

// ============================================================================
// Helper Functions
// ============================================================================

void WeatherScreen::drawHorizontalLine(int x, int y, int width)
{
    display.fillRect(x, y, width, 1, GxEPD_LIGHTGREY);
}

void WeatherScreen::drawWeatherIcon(int x, int y, int size, int weatherCode, bool isDay)
{
    const uint8_t* icon = getWeatherIcon(weatherCode, isDay);
    
    // Scale icon to desired size (icons are 64x64)
    float scale = (float)size / 64.0f;
    
    // Draw icon bitmap
    for (int iy = 0; iy < 64; iy++) {
        for (int ix = 0; ix < 64; ix++) {
            int byteIdx = (iy * 64 + ix) / 8;
            int bitIdx = 7 - (ix % 8);
            
            if (pgm_read_byte(&icon[byteIdx]) & (1 << bitIdx)) {
                int dx = x + (int)(ix * scale);
                int dy = y + (int)(iy * scale);
                
                if (scale >= 1.5f) {
                    // Draw scaled pixel as larger block
                    int blockSize = (int)scale;
                    display.fillRect(dx, dy, blockSize, blockSize, GxEPD_BLACK);
                } else {
                    display.drawPixel(dx, dy, GxEPD_BLACK);
                }
            }
        }
    }
}

void WeatherScreen::drawBatteryIcon(int x, int y, int level)
{
    // Battery outline
    int w = 24, h = 12;
    display.fillRect(x, y, w, h, GxEPD_WHITE);
    display.fillRect(x, y, w, 1, GxEPD_BLACK);  // Top
    display.fillRect(x, y + h - 1, w, 1, GxEPD_BLACK);  // Bottom
    display.fillRect(x, y, 1, h, GxEPD_BLACK);  // Left
    display.fillRect(x + w - 1, y, 1, h, GxEPD_BLACK);  // Right
    display.fillRect(x + w, y + 3, 2, 6, GxEPD_BLACK);  // Tip
    
    // Fill level
    int fillWidth = (w - 4) * level / 100;
    uint16_t color = level > 20 ? GxEPD_BLACK : GxEPD_DARKGREY;
    display.fillRect(x + 2, y + 2, fillWidth, h - 4, color);
}

void WeatherScreen::drawWiFiIcon(int x, int y, int level)
{
    // Simple WiFi bars
    int barWidth = 4;
    int spacing = 2;
    int bars = (level * 4) / 100;  // 0-4 bars
    
    for (int i = 0; i < 4; i++) {
        int barHeight = 4 + i * 3;
        int barX = x + i * (barWidth + spacing);
        int barY = y + (16 - barHeight);
        
        uint16_t color = (i < bars) ? GxEPD_BLACK : GxEPD_LIGHTGREY;
        display.fillRect(barX, barY, barWidth, barHeight, color);
    }
}

const char* WeatherScreen::getTimePeriodName(int hour)
{
    if (hour >= 5 && hour < 12) return Localization::get(STR_TIME_MORNING);
    if (hour >= 12 && hour < 17) return Localization::get(STR_TIME_AFTERNOON);
    if (hour >= 17 && hour < 21) return Localization::get(STR_TIME_EVENING);
    return Localization::get(STR_TIME_NIGHT);
}
