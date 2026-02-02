#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"

class TimeSync
{
public:
    void initSync(String tz)
    {
        configTzTime(tz.c_str(), ntpServer);
    }

    bool hasTime()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            delay(100);
            return false;
        }
        return true;
    }

private:
    const char *ntpServer = "time.google.com";
};
