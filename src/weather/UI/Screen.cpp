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

// ============================================================================
// ULTIMATE WEATHER DASHBOARD - Maximum Information Density
// ============================================================================

void WeatherScreen::drawWeatherScreen(WeatherScreenData_t& screenData, WeatherData_t& weatherData)
{
    display.beginDraw();
    display.firstPage();
    do {
        display.clear();
        
        int screenW = display.getDisplayWidth();   // 640
        int screenH = display.getDisplayHeight();  // 960
        
        int y = 0;
        
        // ========== HEADER BAR (compact) ==========
        drawHeaderBar(0, y, screenW, screenData, weatherData.current);
        y += 50;
        
        // ========== MAIN CURRENT WEATHER (hero section) ==========
        drawCurrentWeatherHero(0, y, screenW, 180, weatherData.current);
        y += 185;
        
        // ========== WEATHER DETAILS GRID (2x3 cards) ==========
        drawWeatherDetailsGrid(0, y, screenW, 120, weatherData.current, weatherData.daily[0]);
        y += 125;
        
        // ========== HOURLY FORECAST (next 8 hours) ==========
        drawHourlyTimeline(0, y, screenW, 130, weatherData.hourly, weatherData.hourlyCount, screenData.currentTime);
        y += 135;
        
        // ========== 7-DAY FORECAST ==========
        draw7DayForecast(0, y, screenW, 290, weatherData.daily, weatherData.dailyCount);
        y += 295;
        
        // ========== SUN & MOON INFO ==========
        drawSunMoonInfo(0, y, screenW, 70, weatherData.daily[0], screenData.currentTime);
        
    } while (display.nextPage());
    display.endDraw();
}

// ============================================================================
// Header Bar - Location, Date, Time, Battery, WiFi
// ============================================================================

void WeatherScreen::drawHeaderBar(int x, int y, int width, WeatherScreenData_t& screenData, CurrentWeather_t& current)
{
    struct tm timeinfo;
    localtime_r(&screenData.currentTime, &timeinfo);
    
    // INVERTED header bar - black background, white text
    display.fillRect(x, y, width, 48, GxEPD_BLACK);
    
    // Location name (left side) - WHITE on black
    display.drawText(MEDIUM, screenData.locationName, x + MARGIN, y + 12, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    
    // Current time (center) - large WHITE
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    display.drawText(MEDIUM, timeStr, width / 2, y + 12,
        CENTER, LEADING, 0, 0, GxEPD_WHITE);
    
    // Date (right of center) - WHITE
    String dateStr = String(timeinfo.tm_mday) + ". " + 
                     String(Localization::getMonthShort(timeinfo.tm_mon));
    display.drawText(SMALL, dateStr, width / 2 + 60, y + 15,
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    
    // Status icons (right side) - drawn in white
    int iconX = width - MARGIN;
    
    // Battery
    drawBatteryIconInverted(iconX - 30, y + 18, screenData.batteryLevel);
    iconX -= 45;
    
    // WiFi
    drawWiFiIconInverted(iconX - 24, y + 16, screenData.wifiSignalLevel);
}

// ============================================================================
// Current Weather Hero - Large temperature, icon, description
// ============================================================================

void WeatherScreen::drawCurrentWeatherHero(int x, int y, int width, int height, CurrentWeather_t& current)
{
    bool isDay = current.isDay;
    int centerX = x + width / 2;
    
    // Large weather icon (left)
    int iconSize = 140;
    int iconX = x + MARGIN + 20;
    int iconY = y + (height - iconSize) / 2;
    drawWeatherIcon(iconX, iconY, iconSize, current.weatherCode, isDay);
    
    // Giant temperature (center-right)
    int tempX = x + 200;
    int tempY = y + 10;
    String tempStr = String((int)round(current.temperature)) + "°";
    display.drawText(HUGE, tempStr, tempX, tempY, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Weather description (below temp)
    String weatherDesc = getWeatherDescription(current.weatherCode);
    display.drawText(MEDIUM, weatherDesc, tempX, tempY + 90, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Feels like (below description) - black text, no grey
    String feelsLike = String(Localization::get(STR_WEATHER_FEELS_LIKE)) + " " + 
                       String((int)round(current.apparentTemperature)) + "°";
    display.drawText(SMALL, feelsLike, tempX, tempY + 125, LEADING, LEADING, 0, 0, GxEPD_BLACK);
    
    // Day/Night indicator in inverted box
    int indicatorW = 50;
    int indicatorH = 30;
    int indicatorX = width - MARGIN - indicatorW;
    int indicatorY = y + 10;
    display.fillRect(indicatorX, indicatorY, indicatorW, indicatorH, GxEPD_BLACK);
    String dayNight = isDay ? "DEN" : "NOC";
    display.drawText(TINY, dayNight, indicatorX + indicatorW/2, indicatorY + 8, CENTER, LEADING, 0, 0, GxEPD_WHITE);
}

// ============================================================================
// Weather Details Grid - 6 cards with key metrics
// ============================================================================

void WeatherScreen::drawWeatherDetailsGrid(int x, int y, int width, int height, CurrentWeather_t& current, DailyForecast_t& today)
{
    int cardWidth = (width - 2 * MARGIN - 10) / 3;  // 3 cards per row
    int cardHeight = (height - 5) / 2;  // 2 rows
    int spacing = 5;
    
    int startX = x + MARGIN;
    
    // Row 1: Wind, Humidity, Precipitation
    drawMetricCard(startX, y, cardWidth, cardHeight, 
        Localization::get(STR_LABEL_WIND), 
        String((int)current.windSpeed) + " km/h",
        getWindDirection(current.windDirection));
    
    drawMetricCard(startX + cardWidth + spacing, y, cardWidth, cardHeight,
        Localization::get(STR_LABEL_HUMIDITY),
        String(current.humidity) + "%",
        current.humidity > 70 ? "Vysoká" : (current.humidity < 30 ? "Nízká" : "OK"));
    
    drawMetricCard(startX + 2 * (cardWidth + spacing), y, cardWidth, cardHeight,
        Localization::get(STR_LABEL_PRECIPITATION),
        String(current.precipitation, 1) + " mm",
        String(today.precipitationProbability) + "% prob.");
    
    // Row 2: High/Low, Rain Chance, UV (estimated)
    int row2Y = y + cardHeight + spacing;
    
    drawMetricCard(startX, row2Y, cardWidth, cardHeight,
        "Max/Min",
        String((int)round(today.tempMax)) + "° / " + String((int)round(today.tempMin)) + "°",
        "");
    
    drawMetricCard(startX + cardWidth + spacing, row2Y, cardWidth, cardHeight,
        "Srážky dnes",
        String(today.precipitationSum, 1) + " mm",
        String(today.precipitationProbability) + "%");
    
    // UV Index estimation
    int uvIndex = estimateUVIndex(current.weatherCode, current.isDay);
    drawMetricCard(startX + 2 * (cardWidth + spacing), row2Y, cardWidth, cardHeight,
        Localization::get(STR_LABEL_UV_INDEX),
        String(uvIndex),
        getUVLevelText(uvIndex));
}

void WeatherScreen::drawMetricCard(int x, int y, int w, int h, String label, String value, String subtext)
{
    // Card with BLACK border, no grey
    display.fillRect(x, y, w, h, GxEPD_WHITE);
    // Black border - 2px thick for visibility
    display.fillRect(x, y, w, 2, GxEPD_BLACK);         // Top
    display.fillRect(x, y + h - 2, w, 2, GxEPD_BLACK); // Bottom
    display.fillRect(x, y, 2, h, GxEPD_BLACK);         // Left
    display.fillRect(x + w - 2, y, 2, h, GxEPD_BLACK); // Right
    
    int centerX = x + w / 2;
    
    // Label (top) - inverted mini header
    display.fillRect(x + 2, y + 2, w - 4, 16, GxEPD_BLACK);
    display.drawText(EXTRA_SMALL, label, centerX, y + 4, CENTER, LEADING, 0, 0, GxEPD_WHITE);
    
    // Value (center, larger) - BLACK
    display.drawText(MEDIUM, value, centerX, y + 24, CENTER, LEADING, 0, 0, GxEPD_BLACK);
    
    // Subtext (bottom) - BLACK, smaller
    if (subtext.length() > 0) {
        display.drawText(EXTRA_SMALL, subtext, centerX, y + 45, CENTER, LEADING, 0, 0, GxEPD_BLACK);
    }
}

// ============================================================================
// Hourly Timeline - Next 8 hours with temperatures and icons
// ============================================================================

void WeatherScreen::drawHourlyTimeline(int x, int y, int width, int height, HourlyForecast_t* hourly, int count, time_t currentTime)
{
    // Section title - inverted
    display.fillRect(x, y, width, 22, GxEPD_BLACK);
    display.drawText(SMALL, Localization::get(STR_WEATHER_HOURLY), x + MARGIN, y + 3, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    
    int contentY = y + 28;
    int hoursToShow = min(count, 8);
    int columnWidth = (width - 2 * MARGIN) / hoursToShow;
    int iconSize = 36;
    
    struct tm currentTm;
    localtime_r(&currentTime, &currentTm);
    int currentHour = currentTm.tm_hour;
    
    for (int i = 0; i < hoursToShow; i++) {
        int colX = x + MARGIN + i * columnWidth + columnWidth / 2;
        
        // Hour label - BLACK text
        int hour = (currentHour + i + 1) % 24;
        char hourStr[8];
        sprintf(hourStr, "%02d:00", hour);
        display.drawText(EXTRA_SMALL, hourStr, colX, contentY, CENTER, LEADING, 0, 0, GxEPD_BLACK);
        
        if (i < count) {
            HourlyForecast_t& h = hourly[i];
            
            // Weather icon
            bool isDay = (h.hour >= 6 && h.hour < 20);
            drawWeatherIcon(colX - iconSize/2, contentY + 16, iconSize, h.weatherCode, isDay);
            
            // Temperature - BLACK
            String tempStr = String((int)round(h.temperature)) + "°";
            display.drawText(SMALL, tempStr, colX, contentY + 56, CENTER, LEADING, 0, 0, GxEPD_BLACK);
            
            // Precipitation probability (if > 0) - in small inverted box
            if (h.precipitationProbability > 0) {
                String probStr = String(h.precipitationProbability) + "%";
                int probW = 30;
                display.fillRect(colX - probW/2, contentY + 76, probW, 14, GxEPD_BLACK);
                display.drawText(EXTRA_SMALL, probStr, colX, contentY + 77, CENTER, LEADING, 0, 0, GxEPD_WHITE);
            }
        }
    }
    
    // Separator line - BLACK
    display.fillRect(x + MARGIN, y + height - 2, width - 2 * MARGIN, 2, GxEPD_BLACK);
}

// ============================================================================
// 7-Day Forecast - Full week with detailed info
// ============================================================================

void WeatherScreen::draw7DayForecast(int x, int y, int width, int height, DailyForecast_t* daily, int count)
{
    // Section title - inverted
    display.fillRect(x, y, width, 22, GxEPD_BLACK);
    display.drawText(SMALL, "7 " + String(Localization::get(STR_WEATHER_DAILY)), x + MARGIN, y + 3, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    
    int contentY = y + 26;
    int daysToShow = min(count, 7);
    int rowHeight = (height - 32) / daysToShow;
    int iconSize = 32;
    
    for (int d = 0; d < daysToShow; d++) {
        DailyForecast_t& day = daily[d];
        int rowY = contentY + d * rowHeight;
        
        // Alternate row backgrounds - INVERTED rows (every other)
        if (d % 2 == 1) {
            display.fillRect(x, rowY, width, rowHeight, GxEPD_BLACK);
        }
        
        bool inverted = (d % 2 == 1);
        uint16_t textColor = inverted ? GxEPD_WHITE : GxEPD_BLACK;
        
        int colX = x + MARGIN + 5;
        
        // Day name (Today, Tomorrow, or day name)
        String dayName;
        if (d == 0) {
            dayName = Localization::get(STR_LABEL_TODAY);
        } else if (d == 1) {
            dayName = Localization::get(STR_LABEL_TOMORROW);
        } else {
            dayName = Localization::getDayShort(day.dayOfWeek);
        }
        display.drawText(SMALL, dayName, colX, rowY + 6, LEADING, LEADING, 0, 0, textColor);
        
        // Date
        String dateStr = String(day.dayOfMonth) + "." + String(day.month + 1) + ".";
        display.drawText(EXTRA_SMALL, dateStr, colX + 70, rowY + 9, LEADING, LEADING, 0, 0, textColor);
        
        // Weather icon - need inverted version for dark rows
        if (inverted) {
            drawWeatherIconInverted(colX + 120, rowY + 3, iconSize, day.weatherCode, true);
        } else {
            drawWeatherIcon(colX + 120, rowY + 3, iconSize, day.weatherCode, true);
        }
        
        // Weather description (short)
        String desc = getWeatherDescriptionShort(day.weatherCode);
        display.drawText(EXTRA_SMALL, desc, colX + 165, rowY + 11, LEADING, LEADING, 0, 0, textColor);
        
        // High temperature
        String highStr = String((int)round(day.tempMax)) + "°";
        display.drawText(SMALL, highStr, width - MARGIN - 90, rowY + 6, TRAILING, LEADING, 0, 0, textColor);
        
        // Low temperature (slightly smaller emphasis)
        String lowStr = String((int)round(day.tempMin)) + "°";
        display.drawText(TINY, lowStr, width - MARGIN - 50, rowY + 9, TRAILING, LEADING, 0, 0, textColor);
        
        // Precipitation chance
        if (day.precipitationProbability > 0) {
            String probStr = String(day.precipitationProbability) + "%";
            display.drawText(EXTRA_SMALL, probStr, width - MARGIN - 5, rowY + 9, TRAILING, LEADING, 0, 0, textColor);
        }
    }
}

// ============================================================================
// Sun & Moon Info - Sunrise, Sunset, Day length
// ============================================================================

void WeatherScreen::drawSunMoonInfo(int x, int y, int width, int height, DailyForecast_t& today, time_t currentTime)
{
    // INVERTED footer - black background, white text
    display.fillRect(x, y, width, height, GxEPD_BLACK);
    
    int thirdWidth = width / 3;
    int contentY = y + 12;
    
    // Parse times
    String sunriseTime = today.sunrise;
    String sunsetTime = today.sunset;
    int tIdx = sunriseTime.indexOf('T');
    if (tIdx > 0) sunriseTime = sunriseTime.substring(tIdx + 1, tIdx + 6);
    tIdx = sunsetTime.indexOf('T');
    if (tIdx > 0) sunsetTime = sunsetTime.substring(tIdx + 1, tIdx + 6);
    
    // Sunrise (left) - white text on black
    display.drawText(EXTRA_SMALL, Localization::get(STR_LABEL_SUNRISE), x + MARGIN, contentY, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    display.drawText(MEDIUM, sunriseTime, x + MARGIN, contentY + 18, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    
    // Day length (center)
    int sunriseH = sunriseTime.substring(0, 2).toInt();
    int sunriseM = sunriseTime.substring(3, 5).toInt();
    int sunsetH = sunsetTime.substring(0, 2).toInt();
    int sunsetM = sunsetTime.substring(3, 5).toInt();
    int dayMinutes = (sunsetH * 60 + sunsetM) - (sunriseH * 60 + sunriseM);
    int dayHours = dayMinutes / 60;
    int dayMins = dayMinutes % 60;
    
    display.drawText(EXTRA_SMALL, "Délka dne", x + thirdWidth + 20, contentY, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    String dayLengthStr = String(dayHours) + "h " + String(dayMins) + "m";
    display.drawText(MEDIUM, dayLengthStr, x + thirdWidth + 20, contentY + 18, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    
    // Sunset (right)
    display.drawText(EXTRA_SMALL, Localization::get(STR_LABEL_SUNSET), x + 2 * thirdWidth + 20, contentY, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    display.drawText(MEDIUM, sunsetTime, x + 2 * thirdWidth + 20, contentY + 18, 
        LEADING, LEADING, 0, 0, GxEPD_WHITE);
    
    // Last update time (far right)
    struct tm timeinfo;
    localtime_r(&currentTime, &timeinfo);
    char updateStr[32];
    sprintf(updateStr, "%s %02d:%02d", Localization::get(STR_WEATHER_UPDATED), timeinfo.tm_hour, timeinfo.tm_min);
    display.drawText(EXTRA_SMALL, updateStr, width - MARGIN - 5, contentY + 25, 
        TRAILING, LEADING, 0, 0, GxEPD_WHITE);
}

// ============================================================================
// Helper Functions
// ============================================================================

String WeatherScreen::getWeatherDescription(int weatherCode) {
    // Map WMO codes to localized descriptions
    if (weatherCode == 0) return Localization::get(STR_WEATHER_CLEAR);
    if (weatherCode <= 3) return Localization::get((StringID)(STR_WEATHER_CLEAR + min(weatherCode, 3)));
    if (weatherCode >= 45 && weatherCode <= 48) return Localization::get(STR_WEATHER_FOG);
    if (weatherCode >= 51 && weatherCode <= 55) return Localization::get(STR_WEATHER_DRIZZLE);
    if (weatherCode >= 61 && weatherCode <= 65) return Localization::get(STR_WEATHER_RAIN);
    if (weatherCode >= 66 && weatherCode <= 67) return Localization::get(STR_WEATHER_FREEZING_RAIN);
    if (weatherCode >= 71 && weatherCode <= 77) return Localization::get(STR_WEATHER_SNOW);
    if (weatherCode >= 80 && weatherCode <= 82) return Localization::get(STR_WEATHER_SHOWERS);
    if (weatherCode >= 85 && weatherCode <= 86) return Localization::get(STR_WEATHER_HEAVY_SNOW);
    if (weatherCode >= 95) return Localization::get(STR_WEATHER_THUNDERSTORM);
    return Localization::get(STR_WEATHER_UNKNOWN);
}

String WeatherScreen::getWeatherDescriptionShort(int weatherCode) {
    // Shorter versions for compact display
    if (weatherCode == 0) return "Jasno";
    if (weatherCode == 1) return "Jasno";
    if (weatherCode == 2) return "Oblačno";
    if (weatherCode == 3) return "Zataženo";
    if (weatherCode >= 45 && weatherCode <= 48) return "Mlha";
    if (weatherCode >= 51 && weatherCode <= 55) return "Mrholení";
    if (weatherCode >= 61 && weatherCode <= 65) return "Déšť";
    if (weatherCode >= 66 && weatherCode <= 67) return "Mráz.déšť";
    if (weatherCode >= 71 && weatherCode <= 77) return "Sníh";
    if (weatherCode >= 80 && weatherCode <= 82) return "Přeháňky";
    if (weatherCode >= 85 && weatherCode <= 86) return "Sněžení";
    if (weatherCode >= 95) return "Bouřka";
    return "?";
}

String WeatherScreen::getWindDirection(int degrees) {
    const char* directions[] = {"S", "SV", "V", "JV", "J", "JZ", "Z", "SZ"};
    int index = ((degrees + 22) % 360) / 45;
    return String(directions[index]);
}

int WeatherScreen::estimateUVIndex(int weatherCode, bool isDay) {
    if (!isDay) return 0;
    if (weatherCode >= 61) return 1;  // Rain/Snow
    if (weatherCode >= 45) return 2;  // Fog
    if (weatherCode == 3) return 3;   // Overcast
    if (weatherCode == 2) return 5;   // Partly cloudy
    if (weatherCode <= 1) return 7;   // Clear/mainly clear
    return 3;
}

String WeatherScreen::getUVLevelText(int uvIndex) {
    if (uvIndex <= 2) return Localization::get(STR_UV_LOW);
    if (uvIndex <= 5) return Localization::get(STR_UV_MODERATE);
    if (uvIndex <= 7) return Localization::get(STR_UV_HIGH);
    if (uvIndex <= 10) return Localization::get(STR_UV_VERY_HIGH);
    return Localization::get(STR_UV_EXTREME);
}

// Keep old methods for backward compatibility but they are no longer used
void WeatherScreen::drawHeader(int x, int y, int width, const char* locationName, time_t currentTime)
{
    // Legacy - not used in new design
}

void WeatherScreen::drawCurrentWeather(int x, int y, int width, int height, CurrentWeather_t& current)
{
    // Legacy - not used in new design
}

void WeatherScreen::drawTodayForecast(int x, int y, int width, int height, HourlyForecast_t* hourly, int count, time_t currentTime)
{
    // Legacy - replaced by drawHourlyTimeline
}

void WeatherScreen::drawDailyOutlook(int x, int y, int width, int height, DailyForecast_t* daily, int count)
{
    // Legacy - replaced by draw7DayForecast
}

void WeatherScreen::drawFooter(int x, int y, int width, DailyForecast_t& today, int uvIndex)
{
    // Legacy - replaced by drawSunMoonInfo
}

// ============================================================================
// Helper Functions (continued)
// ============================================================================

void WeatherScreen::drawHorizontalLine(int x, int y, int width)
{
    display.fillRect(x, y, width, 2, GxEPD_BLACK);
}

void WeatherScreen::drawWeatherIcon(int x, int y, int size, int weatherCode, bool isDay)
{
    const uint8_t* icon = getWeatherIcon(weatherCode, isDay);
    
    // Scale icon to desired size (icons are 64x64)
    float scale = (float)size / 64.0f;
    
    // Draw icon bitmap - BLACK pixels on white background
    for (int iy = 0; iy < 64; iy++) {
        for (int ix = 0; ix < 64; ix++) {
            int byteIdx = (iy * 64 + ix) / 8;
            int bitIdx = 7 - (ix % 8);
            
            if (pgm_read_byte(&icon[byteIdx]) & (1 << bitIdx)) {
                int dx = x + (int)(ix * scale);
                int dy = y + (int)(iy * scale);
                
                if (scale >= 1.5f) {
                    int blockSize = (int)scale;
                    display.fillRect(dx, dy, blockSize, blockSize, GxEPD_BLACK);
                } else {
                    display.drawPixel(dx, dy, GxEPD_BLACK);
                }
            }
        }
    }
}

void WeatherScreen::drawWeatherIconInverted(int x, int y, int size, int weatherCode, bool isDay)
{
    const uint8_t* icon = getWeatherIcon(weatherCode, isDay);
    
    // Scale icon to desired size (icons are 64x64)
    float scale = (float)size / 64.0f;
    
    // Draw icon bitmap - WHITE pixels on black background
    for (int iy = 0; iy < 64; iy++) {
        for (int ix = 0; ix < 64; ix++) {
            int byteIdx = (iy * 64 + ix) / 8;
            int bitIdx = 7 - (ix % 8);
            
            if (pgm_read_byte(&icon[byteIdx]) & (1 << bitIdx)) {
                int dx = x + (int)(ix * scale);
                int dy = y + (int)(iy * scale);
                
                if (scale >= 1.5f) {
                    int blockSize = (int)scale;
                    display.fillRect(dx, dy, blockSize, blockSize, GxEPD_WHITE);
                } else {
                    display.drawPixel(dx, dy, GxEPD_WHITE);
                }
            }
        }
    }
}

void WeatherScreen::drawBatteryIcon(int x, int y, int level)
{
    // Battery outline - BLACK on white
    int w = 24, h = 12;
    display.fillRect(x, y, w, h, GxEPD_WHITE);
    display.fillRect(x, y, w, 1, GxEPD_BLACK);  // Top
    display.fillRect(x, y + h - 1, w, 1, GxEPD_BLACK);  // Bottom
    display.fillRect(x, y, 1, h, GxEPD_BLACK);  // Left
    display.fillRect(x + w - 1, y, 1, h, GxEPD_BLACK);  // Right
    display.fillRect(x + w, y + 3, 2, 6, GxEPD_BLACK);  // Tip
    
    // Fill level - BLACK only
    int fillWidth = (w - 4) * level / 100;
    display.fillRect(x + 2, y + 2, fillWidth, h - 4, GxEPD_BLACK);
}

void WeatherScreen::drawBatteryIconInverted(int x, int y, int level)
{
    // Battery outline - WHITE on black background
    int w = 24, h = 12;
    display.fillRect(x, y, w, 1, GxEPD_WHITE);  // Top
    display.fillRect(x, y + h - 1, w, 1, GxEPD_WHITE);  // Bottom
    display.fillRect(x, y, 1, h, GxEPD_WHITE);  // Left
    display.fillRect(x + w - 1, y, 1, h, GxEPD_WHITE);  // Right
    display.fillRect(x + w, y + 3, 2, 6, GxEPD_WHITE);  // Tip
    
    // Fill level - WHITE
    int fillWidth = (w - 4) * level / 100;
    display.fillRect(x + 2, y + 2, fillWidth, h - 4, GxEPD_WHITE);
}

void WeatherScreen::drawWiFiIcon(int x, int y, int level)
{
    // Simple WiFi bars - BLACK
    int barWidth = 4;
    int spacing = 2;
    int bars = (level * 4) / 100;  // 0-4 bars
    
    for (int i = 0; i < 4; i++) {
        int barHeight = 4 + i * 3;
        int barX = x + i * (barWidth + spacing);
        int barY = y + (16 - barHeight);
        
        // Only show active bars, no grey for inactive
        if (i < bars) {
            display.fillRect(barX, barY, barWidth, barHeight, GxEPD_BLACK);
        } else {
            // Draw empty outline for inactive bars
            display.fillRect(barX, barY, barWidth, 1, GxEPD_BLACK);
            display.fillRect(barX, barY + barHeight - 1, barWidth, 1, GxEPD_BLACK);
            display.fillRect(barX, barY, 1, barHeight, GxEPD_BLACK);
            display.fillRect(barX + barWidth - 1, barY, 1, barHeight, GxEPD_BLACK);
        }
    }
}

void WeatherScreen::drawWiFiIconInverted(int x, int y, int level)
{
    // Simple WiFi bars - WHITE on black
    int barWidth = 4;
    int spacing = 2;
    int bars = (level * 4) / 100;  // 0-4 bars
    
    for (int i = 0; i < 4; i++) {
        int barHeight = 4 + i * 3;
        int barX = x + i * (barWidth + spacing);
        int barY = y + (16 - barHeight);
        
        if (i < bars) {
            display.fillRect(barX, barY, barWidth, barHeight, GxEPD_WHITE);
        } else {
            // Draw empty outline for inactive bars
            display.fillRect(barX, barY, barWidth, 1, GxEPD_WHITE);
            display.fillRect(barX, barY + barHeight - 1, barWidth, 1, GxEPD_WHITE);
            display.fillRect(barX, barY, 1, barHeight, GxEPD_WHITE);
            display.fillRect(barX + barWidth - 1, barY, 1, barHeight, GxEPD_WHITE);
        }
    }
}

const char* WeatherScreen::getTimePeriodName(int hour)
{
    if (hour >= 5 && hour < 12) return Localization::get(STR_TIME_MORNING);
    if (hour >= 12 && hour < 17) return Localization::get(STR_TIME_AFTERNOON);
    if (hour >= 17 && hour < 21) return Localization::get(STR_TIME_EVENING);
    return Localization::get(STR_TIME_NIGHT);
}
