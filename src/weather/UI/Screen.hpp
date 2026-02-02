#pragma once
#include <Arduino.h>
#include "Display.hpp"
#include "../Storage/RadarStorage.hpp"
#include "../API/OpenMeteoAPI.hpp"

typedef struct {
    time_t radarTimestamp;
    time_t currentTime;
    int batteryLevel;
    int wifiSignalLevel;
    bool hasRadarData;
} WeatherScreenData_t;

class WeatherScreen
{
public:
    WeatherScreen();
    void drawBootScreen();
    void drawRadarScreen(WeatherScreenData_t& data, RadarStorage& storage);
    void drawWeatherScreen(WeatherScreenData_t& data, RadarStorage& storage, WeatherData_t& weather);
    void drawStatusScreen(String message);
    void drawErrorScreen(String error);
    void drawConfigScreen(String ssid, String ip);

private:
    Display display;
    void drawHeader(WeatherScreenData_t& data, const String& locationName);
    void drawStatusBar(WeatherScreenData_t& data);
    void drawRadarLegend();
    
    // New layout sections
    void drawRadarMini(WeatherScreenData_t& data, RadarStorage& storage, int x, int y, int w, int h);
    void drawCurrentWeather(WeatherData_t& weather, int x, int y, int w, int h);
    void drawHourlyForecast(WeatherData_t& weather, int x, int y, int w, int h);
    void drawDailyForecast(WeatherData_t& weather, int x, int y, int w, int h);
};
