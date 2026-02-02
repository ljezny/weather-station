#include "WeatherConfiguration.hpp"
#include "../Logging/Logging.hpp"

void WeatherConfiguration::reset()
{
    ssid = "";
    wifiPassword = "";
    timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
    factoryResetFlag = false;
    
    locationName = "Praha";
    latitude = 50.0755;
    longitude = 14.4378;
    refreshIntervalMinutes = 10;
    language = "cs";
    fontScale = FONT_SCALE_NORMAL;
    
    LOGD("Configuration reset to defaults");
}

void WeatherConfiguration::load()
{
    prefs.begin("weather-config", false);
    
    ssid = prefs.getString("ssid", "");
    wifiPassword = prefs.getString("wifi_pass", "");
    timezone = prefs.getString("timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
    factoryResetFlag = prefs.getBool("factory_reset", false);
    
    locationName = prefs.getString("location", "Praha");
    latitude = prefs.getFloat("latitude", 50.0755);
    longitude = prefs.getFloat("longitude", 14.4378);
    refreshIntervalMinutes = prefs.getInt("refresh_min", 10);
    
    // Language
    language = prefs.getString("language", "cs");
    
    // Font scale
    fontScale = (FontScale_t)prefs.getUChar("font_scale", FONT_SCALE_NORMAL);
    
    prefs.end();
    
    LOGD("Configuration loaded");
}

void WeatherConfiguration::save()
{
    prefs.begin("weather-config", false);
    
    prefs.putString("ssid", ssid);
    prefs.putString("wifi_pass", wifiPassword);
    prefs.putString("timezone", timezone);
    prefs.putBool("factory_reset", factoryResetFlag);
    
    prefs.putString("location", locationName);
    prefs.putFloat("latitude", latitude);
    prefs.putFloat("longitude", longitude);
    prefs.putInt("refresh_min", refreshIntervalMinutes);
    prefs.putString("language", language);
    prefs.putUChar("font_scale", (uint8_t)fontScale);
    
    prefs.end();
    
    LOGD("Configuration saved");
}
