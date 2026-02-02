#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WiFiSignal
{
public:
    int getWiFiSignalLevel()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return 0;
        }
        
        int rssi = WiFi.RSSI();
        // Convert RSSI to percentage (0-100)
        return min(max(2 * (rssi + 100), 0), 100);
    }
};
