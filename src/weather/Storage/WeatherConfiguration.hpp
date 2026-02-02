#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "../Localization/Localization.hpp"

#define TIME_ZONE_CODE_LENGTH 64
#define TIME_ZONE_CODE_DEFAULT "Europe/Prague|CET-1CEST,M3.5.0,M10.5.0/3"
#define LOCATION_NAME_LENGTH 64

// Font scale - for accessibility
typedef enum {
    FONT_SCALE_SMALL = 0,   // Smaller fonts - more content visible
    FONT_SCALE_NORMAL = 1,  // Default font sizes
    FONT_SCALE_LARGE = 2    // Larger fonts for better readability
} FontScale_t;

class WeatherConfiguration
{
public:
    void reset();
    void load();
    void save();

    String ssid;
    String wifiPassword;
    String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";  // POSIX timezone string
    bool factoryResetFlag = false;
    
    // Location settings
    String locationName = "Praha";
    float latitude = 50.0755;
    float longitude = 14.4378;
    
    // Refresh interval in minutes (default 10 minutes)
    int refreshIntervalMinutes = 10;
    
    // UI language
    String language = "cs";
    
    // Font scale for accessibility
    FontScale_t fontScale = FONT_SCALE_NORMAL;
    
private:
    Preferences prefs;
};
