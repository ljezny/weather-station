#include "OTAUpdater.hpp"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "../Logging/Logging.hpp"
#include "../consts.h"

OTAUpdater::OTAUpdater() {
    client = new WiFiClientSecure;
    client->setInsecure();
}

OTAUpdater::~OTAUpdater() {
    delete client;
}

void configureClient(HTTPClient& https) {
    https.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    https.setRedirectLimit(0);

    const char *headerKeys[] = {"Location"};
    const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
    https.collectHeaders(headerKeys, headerKeysCount);
}

bool OTAUpdater::check() {
#if !OTA_ENABLED
    return false;
#endif

    String url = "https://www.solar-station.cz/user/documents/weather-station/info.json";
    LOGD("URL: " + url);
    
    String location = url;
    while(!location.isEmpty()) {
        HTTPClient https;
        configureClient(https);
        if (https.begin(location)) {
            location = "";
            int httpCode = https.GET();
            if (httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                location = https.header("Location");
                LOGD("Redirect to:" + location);
            }
            else if (httpCode == HTTP_CODE_OK) {
                String payload = https.getString();
                LOGD(payload);
                DynamicJsonDocument doc(4196);
                deserializeJson(doc, payload);
                bool needsUpdate = false;
                JsonArray arr = doc.as<JsonArray>();
                for (JsonVariant value : arr) {
                    if(value["platform"].as<String>().equals(String(PROJECT))) {
                        LOGD(value.as<String>());
                        targetMd5 = value["md5"].as<String>();
                        if(targetMd5 == "null"){
                            targetMd5 = "";
                        }
                        LOGD("targetMd5:" + targetMd5 + " length: " + targetMd5.length());
                        if(targetMd5.length() == 0 || targetMd5.length() == 32){
                            return !value["version"].as<String>().equals(String(VERSION_NUMBER));
                        } else{
                            LOGD("Error MD5 size");
                            return false;
                        }
                    }
                }
                if(needsUpdate) {
                    update();
                }
            } else {
                LOGD("ERROR: " + https.errorToString(httpCode));
            }
            https.end();
        } else {
            LOGD("Unable to connect.");
        }
    }
    return false;
}

void OTAUpdater::update() { 
    String url = "https://www.solar-station.cz/user/documents/weather-station/" + String(PROJECT) + "/firmware.bin";
    LOGD("URL: " + url);
    String location = url;
    while(!location.isEmpty()) {
        HTTPClient https;
        configureClient(https);

        if (https.begin(location)) {
            location = "";
            int httpCode = https.GET();
            if (httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                location = https.header("Location");
                LOGD("Redirect to:" + location);
            }
            else if (httpCode == HTTP_CODE_OK) {
                LOGD(String(https.getSize()));
                if(Update.begin(https.getSize())) {
                    Update.writeStream(https.getStream());
                    if(targetMd5.length() == 32){
                        LOGD("Update set md5.");
                        Update.setMD5(targetMd5.c_str());
                    }
                    if(Update.end()) {
                        ESP.restart();
                    } else {
                        LOGD("Update end failed.");
                        LOGD(Update.errorString());
                    }
                } else {
                    LOGD("Update begin failed.");
                }
            } else {
                LOGD("ERROR: " + https.errorToString(httpCode));
            }
            https.end();
        } else {
            LOGD("Unable to connect.");
        }
    }
}
