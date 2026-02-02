#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../Logging/Logging.hpp"

// Weather code mapping from Open-Meteo WMO codes
// https://open-meteo.com/en/docs
enum WeatherCode {
    WC_CLEAR = 0,
    WC_MAINLY_CLEAR = 1,
    WC_PARTLY_CLOUDY = 2,
    WC_OVERCAST = 3,
    WC_FOG = 45,
    WC_DEPOSITING_FOG = 48,
    WC_DRIZZLE_LIGHT = 51,
    WC_DRIZZLE_MODERATE = 53,
    WC_DRIZZLE_DENSE = 55,
    WC_FREEZING_DRIZZLE_LIGHT = 56,
    WC_FREEZING_DRIZZLE_DENSE = 57,
    WC_RAIN_SLIGHT = 61,
    WC_RAIN_MODERATE = 63,
    WC_RAIN_HEAVY = 65,
    WC_FREEZING_RAIN_LIGHT = 66,
    WC_FREEZING_RAIN_HEAVY = 67,
    WC_SNOW_SLIGHT = 71,
    WC_SNOW_MODERATE = 73,
    WC_SNOW_HEAVY = 75,
    WC_SNOW_GRAINS = 77,
    WC_RAIN_SHOWERS_SLIGHT = 80,
    WC_RAIN_SHOWERS_MODERATE = 81,
    WC_RAIN_SHOWERS_VIOLENT = 82,
    WC_SNOW_SHOWERS_SLIGHT = 85,
    WC_SNOW_SHOWERS_HEAVY = 86,
    WC_THUNDERSTORM = 95,
    WC_THUNDERSTORM_HAIL_SLIGHT = 96,
    WC_THUNDERSTORM_HAIL_HEAVY = 99
};

// Icon type for e-ink display
enum WeatherIconType {
    ICON_SUN,
    ICON_PARTLY_CLOUDY,
    ICON_CLOUDY,
    ICON_FOG,
    ICON_DRIZZLE,
    ICON_RAIN,
    ICON_HEAVY_RAIN,
    ICON_SNOW,
    ICON_THUNDERSTORM,
    ICON_UNKNOWN
};

// Current weather data
typedef struct {
    float temperature;
    float apparentTemperature;
    int humidity;
    float windSpeed;
    int windDirection;
    int weatherCode;
    bool isDay;
    float precipitation;
} CurrentWeather_t;

// Hourly forecast item
typedef struct {
    int hour;           // 0-23
    float temperature;
    int weatherCode;
    float precipitation;
    int precipitationProbability;
} HourlyForecast_t;

// Daily forecast item
typedef struct {
    int dayOfWeek;      // 0=Sunday, 1=Monday, ...
    int dayOfMonth;
    int month;
    float tempMax;
    float tempMin;
    int weatherCode;
    float precipitationSum;
    int precipitationProbability;
    String sunrise;
    String sunset;
} DailyForecast_t;

// Full weather data structure
typedef struct {
    CurrentWeather_t current;
    HourlyForecast_t hourly[24];     // Next 24 hours
    int hourlyCount;
    DailyForecast_t daily[7];        // Next 7 days
    int dailyCount;
    String locationName;
    float latitude;
    float longitude;
    bool valid;
} WeatherData_t;

class OpenMeteoAPI
{
public:
    OpenMeteoAPI() {}
    
    // Fetch all weather data (current, hourly, daily)
    bool fetchWeatherData(float latitude, float longitude, const String& locationName)
    {
        weatherData.valid = false;
        weatherData.latitude = latitude;
        weatherData.longitude = longitude;
        weatherData.locationName = locationName;
        
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure(); // Skip certificate verification
        
        // Build API URL with all needed parameters
        // Limit hourly to 24 hours to reduce JSON size
        String url = "https://api.open-meteo.com/v1/forecast?"
            "latitude=" + String(latitude, 4) +
            "&longitude=" + String(longitude, 4) +
            "&current=temperature_2m,relative_humidity_2m,apparent_temperature,"
            "precipitation,weather_code,wind_speed_10m,wind_direction_10m,is_day"
            "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation"
            "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
            "precipitation_sum,precipitation_probability_max,sunrise,sunset"
            "&timezone=auto"
            "&forecast_days=7"
            "&forecast_hours=24";  // Limit hourly data to reduce memory usage
        
        LOGD("Fetching weather from: " + url);
        
        http.begin(client, url);
        http.setTimeout(15000);
        
        int httpCode = http.GET();
        LOGD("HTTP Response code: " + String(httpCode));
        
        if (httpCode != HTTP_CODE_OK)
        {
            LOGD("Failed to fetch weather data");
            http.end();
            return false;
        }
        
        String payload = http.getString();
        http.end();
        
        LOGD("Payload size: " + String(payload.length()));
        
        // Parse JSON response - use larger buffer for safety
        DynamicJsonDocument doc(24576);  // 24KB buffer
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error)
        {
            LOGD("JSON parsing failed: " + String(error.c_str()));
            return false;
        }
        
        // Parse current weather
        JsonObject current = doc["current"];
        weatherData.current.temperature = current["temperature_2m"].as<float>();
        weatherData.current.apparentTemperature = current["apparent_temperature"].as<float>();
        weatherData.current.humidity = current["relative_humidity_2m"].as<int>();
        weatherData.current.precipitation = current["precipitation"].as<float>();
        weatherData.current.weatherCode = current["weather_code"].as<int>();
        weatherData.current.windSpeed = current["wind_speed_10m"].as<float>();
        weatherData.current.windDirection = current["wind_direction_10m"].as<int>();
        weatherData.current.isDay = current["is_day"].as<int>() == 1;
        
        // Parse hourly forecast (next 24 hours)
        // With forecast_hours=24, we get exactly 24 hours starting from current hour
        JsonArray hourlyTime = doc["hourly"]["time"];
        JsonArray hourlyTemp = doc["hourly"]["temperature_2m"];
        JsonArray hourlyCode = doc["hourly"]["weather_code"];
        JsonArray hourlyPrecip = doc["hourly"]["precipitation"];
        JsonArray hourlyPrecipProb = doc["hourly"]["precipitation_probability"];
        
        weatherData.hourlyCount = 0;
        int maxHours = min((int)hourlyTime.size(), 24);
        for (int i = 0; i < maxHours; i++)
        {
            String timeStr = hourlyTime[i].as<String>();
            
            // Extract hour from "2024-01-15T14:00"
            int hour = timeStr.substring(11, 13).toInt();
            
            weatherData.hourly[i].hour = hour;
            weatherData.hourly[i].temperature = hourlyTemp[i].as<float>();
            weatherData.hourly[i].weatherCode = hourlyCode[i].as<int>();
            weatherData.hourly[i].precipitation = hourlyPrecip[i].as<float>();
            weatherData.hourly[i].precipitationProbability = hourlyPrecipProb[i].as<int>();
            weatherData.hourlyCount++;
        }
        
        // Parse daily forecast
        JsonArray dailyTime = doc["daily"]["time"];
        JsonArray dailyTempMax = doc["daily"]["temperature_2m_max"];
        JsonArray dailyTempMin = doc["daily"]["temperature_2m_min"];
        JsonArray dailyCode = doc["daily"]["weather_code"];
        JsonArray dailyPrecip = doc["daily"]["precipitation_sum"];
        JsonArray dailyPrecipProb = doc["daily"]["precipitation_probability_max"];
        JsonArray dailySunrise = doc["daily"]["sunrise"];
        JsonArray dailySunset = doc["daily"]["sunset"];
        
        weatherData.dailyCount = 0;
        for (int i = 0; i < 7 && i < dailyTime.size(); i++)
        {
            String dateStr = dailyTime[i].as<String>(); // "2024-01-15"
            
            // Parse date
            int year = dateStr.substring(0, 4).toInt();
            int month = dateStr.substring(5, 7).toInt();
            int day = dateStr.substring(8, 10).toInt();
            
            // Calculate day of week
            struct tm timeinfo = {0};
            timeinfo.tm_year = year - 1900;
            timeinfo.tm_mon = month - 1;
            timeinfo.tm_mday = day;
            mktime(&timeinfo);
            
            weatherData.daily[i].dayOfWeek = timeinfo.tm_wday;
            weatherData.daily[i].dayOfMonth = day;
            weatherData.daily[i].month = month;
            weatherData.daily[i].tempMax = dailyTempMax[i].as<float>();
            weatherData.daily[i].tempMin = dailyTempMin[i].as<float>();
            weatherData.daily[i].weatherCode = dailyCode[i].as<int>();
            weatherData.daily[i].precipitationSum = dailyPrecip[i].as<float>();
            weatherData.daily[i].precipitationProbability = dailyPrecipProb[i].as<int>();
            weatherData.daily[i].sunrise = dailySunrise[i].as<String>().substring(11, 16);
            weatherData.daily[i].sunset = dailySunset[i].as<String>().substring(11, 16);
            weatherData.dailyCount++;
        }
        
        weatherData.valid = true;
        LOGD("Weather data parsed successfully");
        
        return true;
    }
    
    // Get the fetched weather data
    WeatherData_t& getWeatherData()
    {
        return weatherData;
    }
    
    // Convert weather code to icon type
    static WeatherIconType getIconType(int weatherCode, bool isDay = true)
    {
        switch (weatherCode)
        {
            case WC_CLEAR:
            case WC_MAINLY_CLEAR:
                return ICON_SUN;
            
            case WC_PARTLY_CLOUDY:
                return ICON_PARTLY_CLOUDY;
            
            case WC_OVERCAST:
                return ICON_CLOUDY;
            
            case WC_FOG:
            case WC_DEPOSITING_FOG:
                return ICON_FOG;
            
            case WC_DRIZZLE_LIGHT:
            case WC_DRIZZLE_MODERATE:
            case WC_DRIZZLE_DENSE:
            case WC_FREEZING_DRIZZLE_LIGHT:
            case WC_FREEZING_DRIZZLE_DENSE:
                return ICON_DRIZZLE;
            
            case WC_RAIN_SLIGHT:
            case WC_RAIN_MODERATE:
            case WC_RAIN_SHOWERS_SLIGHT:
            case WC_RAIN_SHOWERS_MODERATE:
            case WC_FREEZING_RAIN_LIGHT:
                return ICON_RAIN;
            
            case WC_RAIN_HEAVY:
            case WC_RAIN_SHOWERS_VIOLENT:
            case WC_FREEZING_RAIN_HEAVY:
                return ICON_HEAVY_RAIN;
            
            case WC_SNOW_SLIGHT:
            case WC_SNOW_MODERATE:
            case WC_SNOW_HEAVY:
            case WC_SNOW_GRAINS:
            case WC_SNOW_SHOWERS_SLIGHT:
            case WC_SNOW_SHOWERS_HEAVY:
                return ICON_SNOW;
            
            case WC_THUNDERSTORM:
            case WC_THUNDERSTORM_HAIL_SLIGHT:
            case WC_THUNDERSTORM_HAIL_HEAVY:
                return ICON_THUNDERSTORM;
            
            default:
                return ICON_UNKNOWN;
        }
    }
    
    // Get weather description in Czech
    static String getWeatherDescription(int weatherCode)
    {
        switch (weatherCode)
        {
            case WC_CLEAR: return "Jasno";
            case WC_MAINLY_CLEAR: return "Převážně jasno";
            case WC_PARTLY_CLOUDY: return "Polojasno";
            case WC_OVERCAST: return "Zataženo";
            case WC_FOG:
            case WC_DEPOSITING_FOG: return "Mlha";
            case WC_DRIZZLE_LIGHT: return "Slabé mrholení";
            case WC_DRIZZLE_MODERATE: return "Mrholení";
            case WC_DRIZZLE_DENSE: return "Husté mrholení";
            case WC_FREEZING_DRIZZLE_LIGHT:
            case WC_FREEZING_DRIZZLE_DENSE: return "Mrznoucí mrholení";
            case WC_RAIN_SLIGHT: return "Slabý déšť";
            case WC_RAIN_MODERATE: return "Déšť";
            case WC_RAIN_HEAVY: return "Silný déšť";
            case WC_FREEZING_RAIN_LIGHT:
            case WC_FREEZING_RAIN_HEAVY: return "Mrznoucí déšť";
            case WC_SNOW_SLIGHT: return "Slabé sněžení";
            case WC_SNOW_MODERATE: return "Sněžení";
            case WC_SNOW_HEAVY: return "Husté sněžení";
            case WC_SNOW_GRAINS: return "Sněhová zrna";
            case WC_RAIN_SHOWERS_SLIGHT: return "Slabé přeháňky";
            case WC_RAIN_SHOWERS_MODERATE: return "Přeháňky";
            case WC_RAIN_SHOWERS_VIOLENT: return "Silné přeháňky";
            case WC_SNOW_SHOWERS_SLIGHT:
            case WC_SNOW_SHOWERS_HEAVY: return "Sněhové přeháňky";
            case WC_THUNDERSTORM: return "Bouřka";
            case WC_THUNDERSTORM_HAIL_SLIGHT:
            case WC_THUNDERSTORM_HAIL_HEAVY: return "Bouřka s krupobitím";
            default: return "Neznámé";
        }
    }
    
    // Get short day name in Czech
    static String getDayNameShort(int dayOfWeek)
    {
        const char* days[] = {"Ne", "Po", "Út", "St", "Čt", "Pá", "So"};
        if (dayOfWeek >= 0 && dayOfWeek <= 6)
            return days[dayOfWeek];
        return "?";
    }
    
    // Get full day name in Czech
    static String getDayName(int dayOfWeek)
    {
        const char* days[] = {"Neděle", "Pondělí", "Úterý", "Středa", "Čtvrtek", "Pátek", "Sobota"};
        if (dayOfWeek >= 0 && dayOfWeek <= 6)
            return days[dayOfWeek];
        return "?";
    }

private:
    WeatherData_t weatherData;
};
