/**
 * Weather Station - Weather Forecast Display
 * 
 * Displays weather forecast from Open-Meteo API
 * on e-ink display with deep sleep for battery efficiency.
 * 
 * Hardware: FireBeetle ESP32 + 10.2" e-ink display (960x640)
 * 
 * Architecture ported from calendar-station project.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_partition.h>
#include "weather/consts.h"
#include "weather/Logging/Logging.hpp"

#include "weather/UI/Screen.hpp"
#include "weather/UI/Display102/Display102.hpp"
#include "weather/WebServer/WeatherWebSetupServer.hpp"
#include "weather/WebServer/CaptivePortal.hpp"
#include "weather/Utils/TimeSync.hpp"
#include "weather/Utils/Battery.hpp"
#include "weather/Utils/WiFiSignal.hpp"
#include "weather/Utils/WiFiManager.hpp"
#include "weather/Utils/OTAUpdater.hpp"
#include "weather/Storage/WeatherConfiguration.hpp"
#include "weather/API/OpenMeteoAPI.hpp"
#include "weather/Localization/Localization.hpp"

SET_LOOP_TASK_STACK_SIZE(48 * 1024);

// Post web setup timeout
#define WEB_SETUP_WAIT_TIMEOUT_MS (1000 * 60 * 5) // 5 minutes

// Global objects
WeatherScreen screen;
WeatherConfiguration configuration;
OpenMeteoAPI weatherAPI;
TimeSync timeSync;
Battery battery;
WiFiSignal wifiSignal;
WiFiManager wifiManager;
OTAUpdater otaUpdater;
WeatherWebSetupServer *webServer = NULL;
CaptivePortal *captivePortal = NULL;

// State machine
typedef enum {
    BOOT,
    STATE_INIT,
    STATE_LOW_BATTERY,
    STATE_FACTORY_RESET,
    STATE_FACTORY_RESET_CHECK,
    STATE_WIFI_SETUP,
    STATE_WEB_SETUP,
    STATE_WIFI_CONNECTING,
    STATE_OTA_UPDATE,
    STATE_LOADING_DATA,
    STATE_SHOW_RESULT,
    STATE_POST_WEB_SETUP,
    STATE_INFINITE_SLEEP,
    STATE_DEEP_SLEEP,
#if DEMO
    STATE_DEMO,
#endif
} state_t;

state_t state;

// RTC memory - persists across deep sleep
RTC_DATA_ATTR uint64_t deepSleepCounter = 0;
RTC_DATA_ATTR RemoteLoggerState remoteLoggerState = {LEVEL_NONE, false, 0, ""};
RTC_DATA_ATTR uint8_t wifiFailureCount = 0;

// Current state data
struct WeatherStationState_t {
    int batteryLevel = 0;
    int wifiSignalLevel = 0;
    int nextUpdatePeriodSec = 600;  // Default 10 minutes
    time_t currentTime = 0;
} currentState;

// Flags
bool forceDrawPopup = false;
unsigned long webSetupWaitStartTime = 0;

// ============================================================================
// Helper Functions
// ============================================================================

bool isTimeout()
{
    return millis() > MAX_WAKEUP_TIME_MS;
}

bool isPowerOnReset()
{
    esp_reset_reason_t reason = esp_reset_reason();
    return reason == ESP_RST_POWERON || 
           reason == ESP_RST_EXT || 
           reason == ESP_RST_SW || 
           reason == ESP_RST_PANIC ||
           reason == ESP_RST_WDT ||
           reason == ESP_RST_BROWNOUT;
}

bool isDeepSleepWakeup()
{
    return esp_reset_reason() == ESP_RST_DEEPSLEEP;
}

String getESPId()
{
    return String(ESP.getEfuseMac());
}

/**
 * @brief Erase the LittleFS partition directly using ESP-IDF partition API.
 */
bool eraseLittleFSPartition()
{
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
    
    if (partition == NULL) {
        LOGD("[FS] Could not find LittleFS partition!");
        return false;
    }
    
    LOGD("[FS] Found partition at offset 0x" + String(partition->address, HEX));
    
    esp_err_t err = esp_partition_erase_range(partition, 0, partition->size);
    if (err != ESP_OK) {
        LOGD("[FS] Failed to erase partition");
        return false;
    }
    
    LOGD("[FS] Partition erased successfully");
    return true;
}

/**
 * @brief Ensure LittleFS is healthy. Erase and reformat only if corrupted.
 */
void ensureLittleFSHealthy()
{
    LOGD("[FS] Checking LittleFS health...");
    
    bool mounted = LittleFS.begin(true);
    
    if (!mounted) {
        LOGD("[FS] LittleFS mount failed, trying partition erase...");
        if (eraseLittleFSPartition()) {
            mounted = LittleFS.begin(true);
        }
        if (!mounted) {
            LOGD("[FS] LittleFS mount failed even after erase!");
            return;
        }
    }
    
    // Test write
    const char* testFile = "/.health_check";
    File f = LittleFS.open(testFile, "w", true);
    if (!f) {
        LOGD("[FS] LittleFS corrupted, erasing...");
        LittleFS.end();
        eraseLittleFSPartition();
        LittleFS.begin(true);
        LittleFS.end();
        return;
    }
    
    size_t written = f.print("ok");
    f.close();
    
    if (written != 2) {
        LOGD("[FS] Write failed, erasing...");
        LittleFS.end();
        eraseLittleFSPartition();
        LittleFS.begin(true);
        LittleFS.end();
        return;
    }
    
    // Verify read
    f = LittleFS.open(testFile, "r");
    if (!f || f.readString() != "ok") {
        LOGD("[FS] Read verify failed, erasing...");
        if (f) f.close();
        LittleFS.end();
        eraseLittleFSPartition();
        LittleFS.begin(true);
        LittleFS.end();
        return;
    }
    f.close();
    
    // Clean up test file
    LittleFS.remove(testFile);
    LittleFS.end();
    
    LOGD("[FS] LittleFS healthy");
}

void logMemory()
{
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime < 10000) return;
    lastLogTime = millis();
    
    LOGD("Free heap: " + String(ESP.getFreeHeap()) + 
         ", Max alloc: " + String(ESP.getMaxAllocHeap()));
}

// ============================================================================
// Captive Portal & Web Server
// ============================================================================

void startCaptivePortal()
{
    LOGD("Starting captive portal...");
    captivePortal = new CaptivePortal();
    captivePortal->start();
}

void stopCaptivePortal()
{
    if (captivePortal == NULL) return;
    captivePortal->stop();
    delete captivePortal;
    captivePortal = NULL;
    LOGD("Captive portal stopped.");
}

void startWebServer()
{
    LOGD("Starting web server...");
    webServer = new WeatherWebSetupServer(&configuration);
    webServer->begin();
}

void stopWebServer()
{
    if (webServer == NULL) return;
    webServer->end();
    delete webServer;
    webServer = NULL;
    LOGD("Web server stopped.");
}

// ============================================================================
// Time Sync
// ============================================================================

void initTimeSync()
{
    String timezone = configuration.timezone;
    LOGD("Timezone: " + timezone);
    timeSync.initSync(timezone.c_str());
}

// ============================================================================
// Deep Sleep
// ============================================================================

void startDeepSleep(uint64_t seconds)
{
    deepSleepCounter++;
    LOGD("Entering deep sleep for " + String((int)seconds) + " seconds (counter: " + 
         String((int)deepSleepCounter) + ")");
    
    if (seconds > 0) {
        esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
    }
    esp_deep_sleep_start();
}

// ============================================================================
// Data Fetching
// ============================================================================

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
    
    if (success) {
        WeatherData_t& data = weatherAPI.getWeatherData();
        LOGD("Weather fetched: " + String(data.current.temperature) + "°C, " + 
             OpenMeteoAPI::getWeatherDescription(data.current.weatherCode));
    } else {
        LOGD("Failed to fetch weather data");
    }
    
    return success;
}

int computeNextUpdatePeriod()
{
    // Use configured refresh interval
    return configuration.refreshIntervalMinutes * 60;
}

// ============================================================================
// Display Functions
// ============================================================================

void displayWeather()
{
    LOGD("Displaying weather screen");
    LOGD("Free heap before display: " + String(ESP.getMaxAllocHeap()));
    
    WeatherScreenData_t screenData;
    screenData.currentTime = currentState.currentTime;
    screenData.batteryLevel = currentState.batteryLevel;
    screenData.wifiSignalLevel = currentState.wifiSignalLevel;
    screenData.locationName = configuration.locationName;
    
    WeatherData_t& weatherData = weatherAPI.getWeatherData();
    
    screen.drawWeatherScreen(screenData, weatherData);
}

#if DEMO
void fillDemoData()
{
    // Set demo time
    struct tm timeinfo = {0};
    timeinfo.tm_year = 2025 - 1900;
    timeinfo.tm_mon = 6 - 1;  // June
    timeinfo.tm_mday = 15;
    timeinfo.tm_hour = 14;
    timeinfo.tm_min = 30;
    time_t t = mktime(&timeinfo);
    struct timeval tv = {t, 0};
    settimeofday(&tv, NULL);
    
    currentState.currentTime = t;
    currentState.batteryLevel = 85;
    currentState.wifiSignalLevel = 90;
    
    // Demo weather data
    // (API will return demo data in demo mode)
}
#endif

// ============================================================================
// State Machine
// ============================================================================

void onEntering(state_t newState)
{
    LOGD("Entering state: " + String(newState));
    
    switch (newState) {
    case STATE_INIT:
        if (!isDeepSleepWakeup()) {
            screen.drawBootScreen();
        }
        break;
        
#if DEMO
    case STATE_DEMO:
        fillDemoData();
        displayWeather();
        break;
#endif
        
    case STATE_LOW_BATTERY:
        screen.drawPopup({
            Localization::get(STR_STATUS_LOW_BATTERY),
            "",
            Localization::get(STR_STATUS_CONNECT_CHARGER),
            0,
            false
        });
        break;
        
    case STATE_FACTORY_RESET_CHECK:
        // Popup drawn in updateState after flag is set
        break;
        
    case STATE_FACTORY_RESET:
        screen.drawBootScreen();
        if (!isDeepSleepWakeup()) {
            screen.drawPopup({
                Localization::get(STR_STATUS_FACTORY_RESET_PROGRESS),
                "",
                "",
                0,
                false
            });
        }
        break;
        
    case STATE_WIFI_SETUP:
        startCaptivePortal();
        if (!isDeepSleepWakeup() || forceDrawPopup) {
            forceDrawPopup = false;
            screen.drawPopup({
                Localization::get(STR_STATUS_CONNECT_WIFI),
                captivePortal->getWiFiQR(),
                captivePortal->getWiFiName(),
                0,
                false
            });
        }
        break;
        
    case STATE_WEB_SETUP:
        startWebServer();
        if (!isDeepSleepWakeup()) {
            screen.drawPopup({
                String(Localization::get(STR_STATUS_OPEN_BROWSER)) + " " + webServer->getWebServerSetupURL(),
                webServer->getWebServerSetupURL(),
                "",
                0,
                false
            });
        }
        break;
        
    case STATE_WIFI_CONNECTING:
        if (!isDeepSleepWakeup()) {
            screen.drawPopup({
                Localization::get(STR_STATUS_CONNECTING),
                "",
                configuration.ssid,
                0,
                false
            });
        }
        wifiManager.begin();
        wifiManager.startConnection(configuration.ssid, configuration.wifiPassword);
        break;
        
    case STATE_OTA_UPDATE:
        if (!isDeepSleepWakeup()) {
            screen.drawPopup({
                Localization::get(STR_STATUS_UPDATING_FIRMWARE),
                "",
                "",
                0,
                false
            });
        }
        break;
        
    case STATE_LOADING_DATA:
        if (!isDeepSleepWakeup()) {
            screen.drawPopup({
                Localization::get(STR_STATUS_LOADING_DATA),
                "",
                "",
                0,
                false
            });
        }
        initTimeSync();
        break;
        
    case STATE_POST_WEB_SETUP:
        startCaptivePortal();
        startWebServer();
        webSetupWaitStartTime = millis();
        screen.drawWebSetupPopupOverlay(
            WiFi.softAPSSID(), 
            WiFi.softAPIP().toString(),
            WiFi.SSID(), 
            WiFi.localIP().toString()
        );
        break;
        
    default:
        break;
    }
    
    LOGD("Entered state: " + String(newState));
}

void onLeaving(state_t oldState)
{
    LOGD("Leaving state: " + String(oldState));
    
    switch (oldState) {
    case STATE_WEB_SETUP:
        stopCaptivePortal();
        stopWebServer();
        break;
    case STATE_POST_WEB_SETUP:
        stopCaptivePortal();
        stopWebServer();
        break;
    default:
        break;
    }
    
    LOGD("Left state: " + String(oldState));
}

void moveToState(state_t newState)
{
    onLeaving(state);
    state = newState;
    onEntering(state);
}

void updateState()
{
    switch (state) {
    case BOOT:
#if DEMO
        moveToState(STATE_DEMO);
#else
        moveToState(STATE_INIT);
#endif
        break;
        
    case STATE_INIT:
        if (currentState.batteryLevel < 10) {
            moveToState(STATE_LOW_BATTERY);
        } else if (configuration.factoryResetFlag) {
            moveToState(STATE_FACTORY_RESET);
        } else if (isPowerOnReset()) {
            moveToState(STATE_FACTORY_RESET_CHECK);
        } else if (configuration.ssid.isEmpty()) {
            moveToState(STATE_WIFI_SETUP);
        } else {
            moveToState(STATE_WIFI_CONNECTING);
        }
        break;
        
    case STATE_LOW_BATTERY:
        startDeepSleep(0);  // Infinite sleep
        break;
        
#if DEMO
    case STATE_DEMO:
        startDeepSleep(0);
        break;
#endif
        
    case STATE_FACTORY_RESET_CHECK:
        configuration.factoryResetFlag = true;
        configuration.save();
        
        screen.drawPopup({
            Localization::get(STR_STATUS_STARTING),
            "",
            Localization::get(STR_STATUS_FACTORY_RESET_HINT),
            0,
            false
        });
        
        delay(5000);
        
        configuration.factoryResetFlag = false;
        configuration.save();
        
        if (configuration.ssid.isEmpty()) {
            moveToState(STATE_WIFI_SETUP);
        } else {
            moveToState(STATE_WIFI_CONNECTING);
        }
        break;
        
    case STATE_FACTORY_RESET:
        configuration.reset();
        configuration.save();
        
        LittleFS.begin(false);
        LittleFS.format();
        LittleFS.end();
        
        wifiManager.clearCache();
        moveToState(STATE_INIT);
        break;
        
    case STATE_WIFI_SETUP:
        if (captivePortal->anyClientConnected()) {
            moveToState(STATE_WEB_SETUP);
        } else if (isTimeout()) {
            moveToState(STATE_INFINITE_SLEEP);
        } else if (!configuration.ssid.isEmpty()) {
            moveToState(STATE_WIFI_CONNECTING);
        }
        break;
        
    case STATE_WEB_SETUP:
        if (isTimeout()) {
            moveToState(STATE_INIT);
        } else if (!configuration.ssid.isEmpty() || webServer->isSetupCompleted()) {
            moveToState(STATE_WIFI_CONNECTING);
        }
        break;
        
    case STATE_WIFI_CONNECTING:
        if (isTimeout()) {
            moveToState(STATE_WIFI_SETUP);
        } else {
            WiFiConnectionResult_t result = wifiManager.updateConnection(
                configuration.ssid, configuration.wifiPassword);
            
            switch (result) {
            case WIFI_CONN_SUCCESS:
                wifiFailureCount = 0;
                LOGD("Connected! IP: " + WiFi.localIP().toString());
                
#if OTA_ENABLED
                // Check OTA only on power-on and with good battery
                if (!isDeepSleepWakeup() && currentState.batteryLevel > 60 && otaUpdater.check()) {
                    moveToState(STATE_OTA_UPDATE);
                } else {
                    moveToState(STATE_LOADING_DATA);
                }
#else
                moveToState(STATE_LOADING_DATA);
#endif
                break;
                
            case WIFI_CONN_FAILED_RETRY:
                delay(100);
                break;
                
            case WIFI_CONN_FAILED_GIVE_UP:
                LOGD("WiFi connection failed after all retries.");
                WiFi.mode(WIFI_OFF);
                
                wifiFailureCount++;
                LOGD("WiFi failure count: " + String(wifiFailureCount));
                
                if (wifiFailureCount >= MAX_WIFI_FAILURES_BEFORE_SETUP) {
                    LOGD("Too many WiFi failures, showing setup screen.");
                    wifiFailureCount = 0;
                    forceDrawPopup = true;
                    moveToState(STATE_WIFI_SETUP);
                } else {
                    LOGD("WiFi failed, will retry after sleep.");
                    currentState.nextUpdatePeriodSec = WIFI_RETRY_SLEEP_SECONDS;
                    moveToState(STATE_SHOW_RESULT);
                }
                break;
                
            case WIFI_CONN_IN_PROGRESS:
            default:
                delay(100);
                break;
            }
        }
        break;
        
    case STATE_OTA_UPDATE:
        otaUpdater.update();
        moveToState(STATE_INFINITE_SLEEP);
        break;
        
    case STATE_LOADING_DATA:
        if (timeSync.hasTime()) {
            wifiManager.updateLeaseStartTime();
            
            // Check remote logging level
            if (remoteLogger.needsLevelCheck()) {
                remoteLogger.setBatteryPercent(currentState.batteryLevel);
                remoteLogger.checkLevel();
            }
            
            currentState.wifiSignalLevel = wifiSignal.getWiFiSignalLevel();
            
            // Fetch weather data
            bool weatherOk = fetchWeatherData();
            
            currentState.nextUpdatePeriodSec = computeNextUpdatePeriod();
            
            // Flush remote logs before disconnecting WiFi
            if (remoteLogger.hasLogsToSend()) {
                remoteLogger.flush();
            }
            
            // Disconnect WiFi to free memory before drawing
            if (isDeepSleepWakeup()) {
                LOGD("Disconnecting WiFi to free memory");
                WiFi.disconnect();
                WiFi.mode(WIFI_OFF);
                delay(100);
            }
            
            if (weatherOk) {
                moveToState(STATE_SHOW_RESULT);
            } else {
                // Even without weather data, show what we have
                moveToState(STATE_SHOW_RESULT);
            }
        } else if (isTimeout()) {
            moveToState(STATE_INFINITE_SLEEP);
        }
        break;
        
    case STATE_SHOW_RESULT:
        currentState.currentTime = time(NULL);
        LOGD("Current time: " + String((int)currentState.currentTime));
        
        displayWeather();
        
        // Show post web setup popup on power-on reset
        if (isPowerOnReset()) {
            moveToState(STATE_POST_WEB_SETUP);
        } else {
            moveToState(STATE_DEEP_SLEEP);
        }
        break;
        
    case STATE_POST_WEB_SETUP:
        if (webServer->isSetupCompleted()) {
            configuration.load();
            screen.drawBootScreen();
            moveToState(STATE_INIT);
        } else if (millis() - webSetupWaitStartTime > WEB_SETUP_WAIT_TIMEOUT_MS) {
            displayWeather();
            moveToState(STATE_DEEP_SLEEP);
        }
        break;
        
    case STATE_DEEP_SLEEP:
        startDeepSleep(currentState.nextUpdatePeriodSec + 10);
        break;
        
    case STATE_INFINITE_SLEEP:
        screen.drawPopup({
            Localization::get(STR_STATUS_PRESS_RESET),
            "",
            "",
            0,
            false
        });
        startDeepSleep(0);
        break;
        
    default:
        break;
    }
}

// ============================================================================
// Arduino Entry Points
// ============================================================================

void setup()
{
    Serial.begin(460800);
    delay(100);
    
    LOGD("=================================");
    LOGD("Weather Station - Forecast + Radar");
    LOGD("Version: " + String(VERSION_NUMBER));
    LOGD("=================================");
    
    // Check and repair LittleFS if corrupted
    ensureLittleFSHealthy();
    
    // Initialize RemoteLogger
    remoteLogger.setServerUrl("http://141.144.227.173");
    remoteLogger.setAppName("weather-station");
    remoteLogger.setStatePointer(&remoteLoggerState);
    remoteLogger.begin(ESP.getEfuseMac(), String(VERSION_NUMBER));
    remoteLogger.newSession();
    
    if (isPowerOnReset()) {
        wifiManager.clearCache();
        wifiFailureCount = 0;
    }
    
    // Initialize battery
    battery.setup();
    currentState.batteryLevel = battery.getBatteryPercent();
    
    // Load configuration
    configuration.load();
    
    // Set language from configuration
    if (configuration.language == "cs") {
        Localization::setLanguage(LANG_CS);
    } else if (configuration.language == "de") {
        Localization::setLanguage(LANG_DE);
    } else if (configuration.language == "pl") {
        Localization::setLanguage(LANG_PL);
    } else {
        Localization::setLanguage(LANG_EN);
    }
    
    initTimeSync();
}

void loop()
{
    updateState();
    logMemory();
    
    if (webServer != NULL) {
        webServer->loop();
    }
    if (captivePortal != NULL) {
        captivePortal->loop();
    }
    
    delay(10);  // yield
}
