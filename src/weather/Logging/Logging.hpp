#pragma once
#include <Arduino.h>
#include "../consts.h"

#if DEBUG
    #define LOGD(message)  do { \
        String msg = String(message); \
        Serial.println("[" + String(millis()) + "]\t" + String(__PRETTY_FUNCTION__) + ":L" + String(__LINE__) + "\t\t-> " + msg); \
        Serial.flush(); \
    } while(0)
#else
    #define LOGD(message) ((void)0)
#endif
