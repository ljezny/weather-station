#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../Logging/Logging.hpp"
#include "../Storage/RadarStorage.hpp"

// Radar tile structure (pro metadata, data jsou v LittleFS)
typedef struct {
    int width;
    int height;
    time_t timestamp;
    bool valid;
} RadarTile_t;

// RainViewer API provides radar data as map tiles
// Czech Republic center approximately: lat 49.8, lon 15.5
// Zoom level 6 covers the whole country nicely

class RadarAPI
{
public:
    RadarAPI() : radarTimestamp(0) {}
    
    // Get latest radar timestamp from RainViewer API
    bool fetchLatestTimestamp()
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure(); // Skip certificate verification for simplicity
        
        String url = "https://api.rainviewer.com/public/weather-maps.json";
        LOGD("Fetching radar metadata from: " + url);
        
        http.begin(client, url);
        http.setTimeout(15000);
        
        int httpCode = http.GET();
        LOGD("HTTP Response code: " + String(httpCode));
        
        if (httpCode != HTTP_CODE_OK)
        {
            LOGD("Failed to fetch radar metadata");
            http.end();
            return false;
        }
        
        String payload = http.getString();
        http.end();
        
        // Parse JSON response
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error)
        {
            LOGD("JSON parsing failed: " + String(error.c_str()));
            return false;
        }
        
        // Get the latest radar frame timestamp
        JsonArray radar = doc["radar"]["past"];
        if (radar.size() > 0)
        {
            radarTimestamp = radar[radar.size() - 1]["time"].as<long>();
            radarPath = radar[radar.size() - 1]["path"].as<String>();
            LOGD("Latest radar timestamp: " + String(radarTimestamp));
            LOGD("Radar path: " + radarPath);
            return true;
        }
        
        return false;
    }
    
    // Download radar tile for Czech Republic
    // Using tile coordinates for zoom level 6 covering Czech Republic
    // Tile URL format: https://tilecache.rainviewer.com{path}/256/{z}/{x}/{y}/2/1_1.png
    // Ukládá přímo do LittleFS přes RadarStorage
    bool downloadRadarTile(RadarStorage& storage, int tileIndex, int zoom, int tileX, int tileY)
    {
        if (radarPath.isEmpty())
        {
            LOGD("No radar path available, fetch timestamp first");
            return false;
        }
        
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        
        // RainViewer tile URL
        // Color scheme: 2 = original
        // Options: 1_1 = smooth + snow
        String url = "https://tilecache.rainviewer.com" + radarPath + 
                     "/256/" + String(zoom) + "/" + String(tileX) + "/" + String(tileY) + "/2/1_1.png";
        
        LOGD("Downloading radar tile: " + url);
        
        http.begin(client, url);
        http.setTimeout(30000);
        
        int httpCode = http.GET();
        LOGD("Tile HTTP Response code: " + String(httpCode));
        
        if (httpCode != HTTP_CODE_OK)
        {
            LOGD("Failed to download radar tile");
            http.end();
            return false;
        }
        
        // Get image data
        int contentLength = http.getSize();
        LOGD("Tile content length: " + String(contentLength));
        
        if (contentLength <= 0 || contentLength > 100000)
        {
            LOGD("Invalid content length");
            http.end();
            return false;
        }
        
        // Otevřeme soubor v LittleFS pro zápis
        File file = storage.openTileForWrite(tileIndex);
        if (!file)
        {
            LOGD("Failed to open file for tile");
            http.end();
            return false;
        }
        
        // Streamujeme data přímo do souboru s malým bufferem
        WiFiClient* stream = http.getStreamPtr();
        uint8_t buffer[512];
        int bytesWritten = 0;
        
        while (http.connected() && bytesWritten < contentLength)
        {
            size_t available = stream->available();
            if (available)
            {
                size_t toRead = min(available, min((size_t)sizeof(buffer), (size_t)(contentLength - bytesWritten)));
                int read = stream->readBytes(buffer, toRead);
                if (read > 0)
                {
                    file.write(buffer, read);
                    bytesWritten += read;
                }
            }
            delay(1);
        }
        
        file.close();
        http.end();
        
        LOGD("Tile saved to LittleFS, bytes: " + String(bytesWritten));
        return (bytesWritten == contentLength);
    }
    
    time_t getRadarTimestamp() { return radarTimestamp; }
    String getRadarPath() { return radarPath; }
    
    // Get tile coordinates for Czech Republic at different zoom levels
    // Zoom 6: Czech Republic fits in ~2x2 tiles
    // Center: approximately lat 49.8, lon 15.5
    static void getCzechTileCoords(int zoom, int& tileX, int& tileY)
    {
        // Convert lat/lon to tile coordinates
        double lat = 49.8;
        double lon = 15.5;
        
        int n = 1 << zoom;
        tileX = (int)((lon + 180.0) / 360.0 * n);
        double latRad = lat * PI / 180.0;
        tileY = (int)((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / PI) / 2.0 * n);
        
        LOGD("Czech tile coords at zoom " + String(zoom) + ": x=" + String(tileX) + ", y=" + String(tileY));
    }

private:
    time_t radarTimestamp;
    String radarPath;
};
