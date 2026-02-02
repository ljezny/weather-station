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
 * Design based on Weather Dashboard mockup:
 * - Header: Title + Location + Date
 * - Current weather: Large icon + temperature + details
 * - Today's Forecast: 4 time periods (Morning, Afternoon, Evening, Night)
 * - 3-Day Outlook: Next 3 days with High/Low
 * - Footer: UV Index + Sunrise/Sunset
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
    static const int MARGIN = 20;
    static const int SPACING = 15;
    
    // Main sections
    void drawHeader(int x, int y, int width, const char* locationName, time_t currentTime);
    void drawCurrentWeather(int x, int y, int width, int height, CurrentWeather_t& current);
    void drawTodayForecast(int x, int y, int width, int height, HourlyForecast_t* hourly, int count, time_t currentTime);
    void drawDailyOutlook(int x, int y, int width, int height, DailyForecast_t* daily, int count);
    void drawFooter(int x, int y, int width, DailyForecast_t& today, int uvIndex);
    
    // Helper functions
    void drawWeatherIcon(int x, int y, int size, int weatherCode, bool isDay);
    void drawBatteryIcon(int x, int y, int level);
    void drawWiFiIcon(int x, int y, int level);
    void drawHorizontalLine(int x, int y, int width);
    
    // Get localized time period name
    const char* getTimePeriodName(int hour);
};
