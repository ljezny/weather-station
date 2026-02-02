#pragma once
#include <Arduino.h>
#include <Preferences.h>

class WeatherConfiguration
{
public:
    String ssid;
    String wifiPassword;
    String timezone;
    int refreshIntervalMinutes;
    
    // Location settings
    float latitude;
    float longitude;
    String locationName;
    
    WeatherConfiguration()
    {
        ssid = "";
        wifiPassword = "";
        timezone = "CET-1CEST,M3.5.0,M10.5.0/3"; // Czech timezone
        refreshIntervalMinutes = 10;
        
        // Default location: Prague
        latitude = 50.0755f;
        longitude = 14.4378f;
        locationName = "Praha";
    }
    
    void load()
    {
        Preferences prefs;
        prefs.begin("weather", true);
        ssid = prefs.getString("ssid", "");
        wifiPassword = prefs.getString("pass", "");
        timezone = prefs.getString("tz", "CET-1CEST,M3.5.0,M10.5.0/3");
        refreshIntervalMinutes = prefs.getInt("refresh", 10);
        latitude = prefs.getFloat("lat", 50.0755f);
        longitude = prefs.getFloat("lon", 14.4378f);
        locationName = prefs.getString("locName", "Praha");
        prefs.end();
    }
    
    void save()
    {
        Preferences prefs;
        prefs.begin("weather", false);
        prefs.putString("ssid", ssid);
        prefs.putString("pass", wifiPassword);
        prefs.putString("tz", timezone);
        prefs.putInt("refresh", refreshIntervalMinutes);
        prefs.putFloat("lat", latitude);
        prefs.putFloat("lon", longitude);
        prefs.putString("locName", locationName);
        prefs.end();
    }
    
    void reset()
    {
        ssid = "";
        wifiPassword = "";
        timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
        refreshIntervalMinutes = 10;
        latitude = 50.0755f;
        longitude = 14.4378f;
        locationName = "Praha";
    }
    
    // Set location from coordinates
    void setLocation(float lat, float lon, const String& name)
    {
        latitude = lat;
        longitude = lon;
        locationName = name;
    }
};
