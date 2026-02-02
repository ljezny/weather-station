/**
 * Weather Station - Radar Map + Weather Forecast
 * 
 * Displays precipitation radar map and weather forecast from Open-Meteo API
 * on e-ink display with deep sleep for battery efficiency.
 * 
 * Hardware: FireBeetle ESP32 + 7.5" e-ink display (800x480)
 */

#include <WiFi.h>
#include <time.h>
#include "weather/consts.h"
#include "weather/Logging/Logging.hpp"
#include "weather/UI/Screen.hpp"
#include "weather/UI/ConfigPortal.hpp"
#include "weather/API/RadarAPI.hpp"
#include "weather/API/OpenMeteoAPI.hpp"
#include "weather/Utils/TimeSync.hpp"
#include "weather/Utils/Battery.hpp"
#include "weather/Utils/WiFiSignal.hpp"
#include "weather/Storage/WeatherConfiguration.hpp"
#include "weather/Storage/RadarStorage.hpp"

SET_LOOP_TASK_STACK_SIZE(48 * 1024);

// Maximum wakeup time before forcing deep sleep
#define MAX_WAKEUP_TIME_MS (1000 * 60 * 3) // 3 minutes

// Button pin for entering config mode
#define CONFIG_BUTTON_PIN 0  // BOOT button on most ESP32 boards

// Global objects
WeatherScreen screen;
WeatherConfiguration configuration;
RadarAPI radarAPI;
OpenMeteoAPI weatherAPI;
RadarStorage radarStorage;
TimeSync timeSync;
Battery battery;
WiFiSignal wifiSignal;
ConfigPortal configPortal;

// State machine
typedef enum {
    STATE_BOOT,
    STATE_CHECK_CONFIG,
    STATE_CONFIG_PORTAL,
    STATE_INIT,
    STATE_WIFI_CONNECTING,
    STATE_LOADING_DATA,
    STATE_SHOW_WEATHER,
    STATE_DEEP_SLEEP,
    STATE_ERROR
} state_t;

state_t currentState = STATE_BOOT;

// Radar tile count (data jsou v LittleFS)
#define MAX_RADAR_TILES 4
int radarTileCount = 0;

// RTC memory for deep sleep persistence
RTC_DATA_ATTR uint64_t bootCount = 0;
RTC_DATA_ATTR time_t lastRadarTime = 0;

bool isTimeout()
{
    return millis() > MAX_WAKEUP_TIME_MS;
}

bool isPowerOnReset()
{
    return esp_reset_reason() == ESP_RST_POWERON || esp_reset_reason() == ESP_RST_WDT;
}

void connectToWiFi()
{
    LOGD("Connecting to WiFi: " + configuration.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(configuration.ssid.c_str(), configuration.wifiPassword.c_str());
}

void startDeepSleep(uint64_t seconds)
{
    LOGD("Entering deep sleep for " + String((int)seconds) + " seconds");
    
    // Zavřít LittleFS
    radarStorage.end();
    
    if (seconds > 0)
    {
        esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
    }
    esp_deep_sleep_start();
}

bool fetchRadarData()
{
    LOGD("Fetching radar data...");
    LOGD("Free heap before fetch: " + String(ESP.getMaxAllocHeap()));
    
    // Inicializuj LittleFS storage
    if (!radarStorage.begin())
    {
        LOGD("Failed to init radar storage");
        return false;
    }
    
    // Smaž staré dlaždice
    radarStorage.clearTiles();
    
    // Get latest radar timestamp
    if (!radarAPI.fetchLatestTimestamp())
    {
        LOGD("Failed to fetch radar timestamp");
        return false;
    }
    
    // Download radar tiles for Czech Republic
    // At zoom level 6, Czech Republic is covered by tiles around (34, 21) and (35, 21)
    int zoom = 6;
    int baseTileX, baseTileY;
    RadarAPI::getCzechTileCoords(zoom, baseTileX, baseTileY);
    
    LOGD("Base tile coords: x=" + String(baseTileX) + " y=" + String(baseTileY));
    
    // Download 2x2 tile grid to cover the country - ukládáme přímo do LittleFS
    radarTileCount = 0;
    for (int dy = 0; dy <= 1 && radarTileCount < MAX_RADAR_TILES; dy++)
    {
        for (int dx = 0; dx <= 1 && radarTileCount < MAX_RADAR_TILES; dx++)
        {
            if (radarAPI.downloadRadarTile(radarStorage, radarTileCount, zoom, baseTileX + dx, baseTileY + dy))
            {
                radarTileCount++;
                LOGD("Downloaded tile " + String(radarTileCount) + ", free heap: " + String(ESP.getMaxAllocHeap()));
            }
            else
            {
                LOGD("Failed to download tile dx=" + String(dx) + " dy=" + String(dy));
            }
            
            delay(100); // Small delay between requests
        }
    }
    
    lastRadarTime = radarAPI.getRadarTimestamp();
    LOGD("Radar data fetched, tile count: " + String(radarTileCount));
    LOGD("Free heap after fetch: " + String(ESP.getMaxAllocHeap()));
    
    return radarTileCount > 0;
}

bool fetchWeatherData()
{
    LOGD("Fetching weather data from Open-Meteo...");
    LOGD("Location: " + configuration.locationName + 
         " (" + String(configuration.latitude, 4) + ", " + String(configuration.longitude, 4) + ")");
    
    bool success = weatherAPI.fetchWeatherData(
        configuration.latitude, 
        configuration.longitude, 
        configuration.locationName
    );
    
    if (success)
    {
        WeatherData_t& data = weatherAPI.getWeatherData();
        LOGD("Weather fetched: " + String(data.current.temperature) + "°C, " + 
             OpenMeteoAPI::getWeatherDescription(data.current.weatherCode));
    }
    else
    {
        LOGD("Failed to fetch weather data");
    }
    
    return success;
}

void displayWeather()
{
    LOGD("Displaying weather screen");
    LOGD("Free heap before display: " + String(ESP.getMaxAllocHeap()));
    
    WeatherScreenData_t screenData;
    screenData.radarTimestamp = lastRadarTime;
    screenData.currentTime = time(NULL);
    screenData.batteryLevel = battery.getBatteryPercent();
    screenData.wifiSignalLevel = wifiSignal.getWiFiSignalLevel();
    screenData.hasRadarData = (radarTileCount > 0);
    
    // Get weather data
    WeatherData_t& weatherData = weatherAPI.getWeatherData();
    
    // Draw the new combined weather screen
    screen.drawWeatherScreen(screenData, radarStorage, weatherData);
}

void onEnterState(state_t newState)
{
    LOGD("Entering state: " + String(newState));
    
    switch (newState)
    {
    case STATE_CHECK_CONFIG:
        // Check if we have valid WiFi configuration
        configuration.load();
        break;
        
    case STATE_CONFIG_PORTAL:
        // Start configuration web server
        configPortal.start(configuration);
        screen.drawConfigScreen(configPortal.getSSID(), configPortal.getAPIP());
        break;
        
    case STATE_INIT:
        if (isPowerOnReset())
        {
            screen.drawBootScreen();
        }
        break;
        
    case STATE_WIFI_CONNECTING:
        screen.drawStatusScreen("Připojování k WiFi...");
        connectToWiFi();
        break;
        
    case STATE_LOADING_DATA:
        screen.drawStatusScreen("Stahuji data počasí...");
        timeSync.initSync(configuration.timezone.c_str());
        break;
        
    case STATE_SHOW_WEATHER:
        displayWeather();
        break;
        
    case STATE_ERROR:
        screen.drawErrorScreen("Nepodařilo se načíst data");
        break;
        
    default:
        break;
    }
}

void moveToState(state_t newState)
{
    currentState = newState;
    onEnterState(newState);
}

void updateStateMachine()
{
    switch (currentState)
    {
    case STATE_BOOT:
        bootCount++;
        LOGD("Boot count: " + String((int)bootCount));
        battery.setup();
        moveToState(STATE_CHECK_CONFIG);
        break;
        
    case STATE_CHECK_CONFIG:
        // Check for config button press or missing WiFi config
        pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
        if (digitalRead(CONFIG_BUTTON_PIN) == LOW || configuration.ssid.length() == 0)
        {
            LOGD("Entering configuration mode");
            moveToState(STATE_CONFIG_PORTAL);
        }
        else
        {
            moveToState(STATE_INIT);
        }
        break;
        
    case STATE_CONFIG_PORTAL:
        // Handle web server requests
        configPortal.handle();
        
        // Check if configuration was submitted
        if (configPortal.isConfigured())
        {
            LOGD("Configuration complete, restarting...");
            configPortal.stop();
            delay(1000);
            ESP.restart();
        }
        
        // Check for timeout (10 minutes)
        if (millis() > 10 * 60 * 1000)
        {
            LOGD("Config portal timeout");
            configPortal.stop();
            moveToState(STATE_DEEP_SLEEP);
        }
        break;
        
    case STATE_INIT:
        moveToState(STATE_WIFI_CONNECTING);
        break;
        
    case STATE_WIFI_CONNECTING:
        if (WiFi.status() == WL_CONNECTED)
        {
            LOGD("WiFi connected, IP: " + WiFi.localIP().toString());
            moveToState(STATE_LOADING_DATA);
        }
        else if (isTimeout())
        {
            LOGD("WiFi connection timeout");
            moveToState(STATE_ERROR);
        }
        else
        {
            delay(100);
        }
        break;
        
    case STATE_LOADING_DATA:
        if (timeSync.hasTime())
        {
            // Fetch weather data first (smaller, faster)
            bool weatherOk = fetchWeatherData();
            
            // Then fetch radar data
            bool radarOk = fetchRadarData();
            
            if (weatherOk)
            {
                // Odpojit WiFi a uvolnit paměť PŘED kreslením
                LOGD("Disconnecting WiFi to free memory before drawing");
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                delay(100);
                LOGD("WiFi disconnected, max alloc: " + String(ESP.getMaxAllocHeap()));
                
                moveToState(STATE_SHOW_WEATHER);
            }
            else
            {
                moveToState(STATE_ERROR);
            }
        }
        else if (isTimeout())
        {
            LOGD("Time sync timeout");
            moveToState(STATE_ERROR);
        }
        break;
        
    case STATE_SHOW_WEATHER:
        // Go to deep sleep after weather is displayed
        moveToState(STATE_DEEP_SLEEP);
        break;
        
    case STATE_ERROR:
        delay(5000);
        moveToState(STATE_DEEP_SLEEP);
        break;
        
    case STATE_DEEP_SLEEP:
        // Sleep for configured interval (default 10 minutes)
        startDeepSleep(configuration.refreshIntervalMinutes * 60);
        break;
    }
}

void setup()
{
    Serial.begin(460800);
    delay(100);
    
    LOGD("=================================");
    LOGD("Weather Station - Forecast + Radar");
    LOGD("Version: " + String(VERSION_NUMBER));
    LOGD("=================================");
}

void loop()
{
    updateStateMachine();
    delay(10);
}
