#pragma once
#include <Arduino.h>
#include "Display102/Display102.hpp"
#include "../API/OpenMeteoAPI.hpp"

/**
 * Screen data for weather display
 */
typedef struct WeatherScreenData {
    time_t currentTime;
    int batteryLevel;
    int wifiSignalLevel;
    String locationName;
} WeatherScreenData_t;

/**
 * Popup info data structure (same as calendar)
 */
typedef struct ScreenInfoData {
    String title;
    String qrCode;
    String subtitle;
    float temperature;
    bool showTemperature;
    
    // Default constructor
    ScreenInfoData() : title(""), qrCode(""), subtitle(""), temperature(0), showTemperature(false) {}
    
    // Constructor for easy initialization
    ScreenInfoData(const char* t, const char* qr, const char* sub, float temp, bool showTemp)
        : title(t), qrCode(qr), subtitle(sub), temperature(temp), showTemperature(showTemp) {}
    
    ScreenInfoData(String t, String qr, String sub, float temp, bool showTemp)
        : title(t), qrCode(qr), subtitle(sub), temperature(temp), showTemperature(showTemp) {}
} ScreenInfoData_t;

/**
 * Weather Screen - handles all display drawing for weather station
 * 
 * ULTIMATE DASHBOARD DESIGN - Maximum Information Density:
 * - Header Bar: Location, Time, Date, Battery, WiFi status
 * - Current Weather Hero: Large icon + giant temperature + description
 * - Weather Details Grid: 6 metric cards (Wind, Humidity, Precipitation, Max/Min, Rain, UV)
 * - Hourly Timeline: Next 8 hours with icons, temps, and precipitation %
 * - 7-Day Forecast: Full week with detailed daily info
 * - Sun & Moon Info: Sunrise, Sunset, Day length, Last update
 */
class WeatherScreen
{
public:
    WeatherScreen();
    
    /**
     * Draw boot/splash screen
     */
    void drawBootScreen();
    
    /**
     * Draw popup overlay (for status messages, QR codes, etc.)
     */
    void drawPopup(ScreenInfoData_t info);
    
    /**
     * Draw the main weather screen
     */
    void drawWeatherScreen(WeatherScreenData_t& screenData, WeatherData_t& weatherData);
    
    /**
     * Draw web setup popup overlay
     */
    void drawWebSetupPopupOverlay(String softAPSSID, String softAPIP, String staSSID, String staIP);
    
    /**
     * Get display reference for direct access if needed
     */
    Display102& getDisplay() { return display; }

private:
    Display102 display;
    
    // Layout constants
    static const int MARGIN = 15;
    static const int SPACING = 10;
    
    // NEW: Ultimate Dashboard Sections
    void drawHeaderBar(int x, int y, int width, WeatherScreenData_t& screenData, CurrentWeather_t& current);
    void drawCurrentWeatherHero(int x, int y, int width, int height, CurrentWeather_t& current);
    void drawWeatherDetailsGrid(int x, int y, int width, int height, CurrentWeather_t& current, DailyForecast_t& today);
    void drawMetricCard(int x, int y, int w, int h, String label, String value, String subtext);
    void drawHourlyTimeline(int x, int y, int width, int height, HourlyForecast_t* hourly, int count, time_t currentTime);
    void draw7DayForecast(int x, int y, int width, int height, DailyForecast_t* daily, int count);
    void drawSunMoonInfo(int x, int y, int width, int height, DailyForecast_t& today, time_t currentTime);
    
    // Legacy sections (kept for compatibility)
    void drawHeader(int x, int y, int width, const char* locationName, time_t currentTime);
    void drawCurrentWeather(int x, int y, int width, int height, CurrentWeather_t& current);
    void drawTodayForecast(int x, int y, int width, int height, HourlyForecast_t* hourly, int count, time_t currentTime);
    void drawDailyOutlook(int x, int y, int width, int height, DailyForecast_t* daily, int count);
    void drawFooter(int x, int y, int width, DailyForecast_t& today, int uvIndex);
    
    // Helper functions
    void drawWeatherIcon(int x, int y, int size, int weatherCode, bool isDay);
    void drawWeatherIconInverted(int x, int y, int size, int weatherCode, bool isDay);
    void drawBatteryIcon(int x, int y, int level);
    void drawBatteryIconInverted(int x, int y, int level);
    void drawWiFiIcon(int x, int y, int level);
    void drawWiFiIconInverted(int x, int y, int level);
    void drawHorizontalLine(int x, int y, int width);
    
    // Text helpers
    String getWeatherDescription(int weatherCode);
    String getWeatherDescriptionShort(int weatherCode);
    String getWindDirection(int degrees);
    int estimateUVIndex(int weatherCode, bool isDay);
    String getUVLevelText(int uvIndex);
    
    // Get localized time period name
    const char* getTimePeriodName(int hour);
};
