#pragma once
#include <Arduino.h>
#include "../consts.h"
#include <RemoteLogger.hpp>

// Undefine RemoteLogger's LOGD to use our own implementation
#ifdef LOGD
    #undef LOGD
#endif

#if DEBUG
    #define LOGD(message)  do { \
        String msg = String(message); \
        Serial.println("[" + String(millis()) + "]\t" + String(__PRETTY_FUNCTION__) + ":L" + String(__LINE__) + "\t\t-> " + msg); \
        Serial.flush(); \
        RLOG_DEBUG(msg); \
    } while(0)
#else
    #define LOGD(message)  RLOG_DEBUG(String(message))
#endif
