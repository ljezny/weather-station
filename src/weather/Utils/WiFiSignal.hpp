#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WiFiSignal
{
public:
    int getWiFiSignalLevel()
    {
        int rssi = WiFi.RSSI();
        if (rssi >= -50)
            return 100;
        if (rssi <= -100)
            return 0;
        return 2 * (rssi + 100);
    }
};
