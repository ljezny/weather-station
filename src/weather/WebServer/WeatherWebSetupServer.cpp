#include <Update.h>
#include <StreamString.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "WeatherWebSetupServer.hpp"
#include "../Storage/WeatherConfiguration.hpp"
#include "../Logging/Logging.hpp"
#include "../Localization/Localization.hpp"
#include "../consts.h"

WeatherWebSetupServer::WeatherWebSetupServer(WeatherConfiguration *configuration)
{
    this->configuration = configuration;
    webServer.addHandler(new EmbeddedFileHandler("/logo.svg", SVG_LOGO, SVG_LOGO_END, "image/svg+xml"));
    webServer.addHandler(new EmbeddedFileHandler("/localization_en.json", LOCALIZATION_EN, LOCALIZATION_EN_END, "application/json"));
    webServer.addHandler(new EmbeddedFileHandler("/localization_de.json", LOCALIZATION_DE, LOCALIZATION_DE_END, "application/json"));
    webServer.addHandler(new EmbeddedFileHandler("/localization_cs.json", LOCALIZATION_CS, LOCALIZATION_CS_END, "application/json"));
    webServer.addHandler(new EmbeddedFileHandler("/localization_pl.json", LOCALIZATION_PL, LOCALIZATION_PL_END, "application/json"));
    webServer.on("/", std::bind(&WeatherWebSetupServer::handleGETSetup, this));
    webServer.on("/style.css", std::bind(&WeatherWebSetupServer::handleCSSStyle, this));
    webServer.on("/wifi", std::bind(&WeatherWebSetupServer::handleWiFiScan, this));
    webServer.on("/config", std::bind(&WeatherWebSetupServer::handleGETConfig, this));
    webServer.on("/complete-setup", HTTP_POST, std::bind(&WeatherWebSetupServer::handlePOSTSetup, this));
    webServer.on("/update-firmware", HTTP_POST, std::bind(&WeatherWebSetupServer::handlePOSTUpdateFirmware, this), std::bind(&WeatherWebSetupServer::handleUploadFirmware, this));
    webServer.onNotFound(std::bind(&WeatherWebSetupServer::handleGETSetup, this));
}

void WeatherWebSetupServer::begin()
{
    webServer.begin();
    setupCompleted = false;
}

void WeatherWebSetupServer::end()
{
    webServer.stop();
    setupCompleted = false;
}

void WeatherWebSetupServer::handlePOSTSetup()
{
    LOGD("Starting handlePOSTSetup with " + String(webServer.args()) + " arguments");
    
    for (uint8_t i = 0; i < webServer.args(); i++)
    {
        String param_name = webServer.argName(i);
        String value = webServer.arg(i);
        LOGD("Parameter: " + param_name + " value: " + value);
        
        if (param_name == "ssid")
        {
            configuration->ssid = value;
        }
        else if (param_name == "wifipassword")
        {
            configuration->wifiPassword = value;
        }
        else if (param_name == "time_zone")
        {
            configuration->timezone = value;
            LOGD("Time zone: " + configuration->timezone);
        }
        else if (param_name == "location_name")
        {
            value.trim();
            configuration->locationName = value;
            LOGD("Location name: " + value);
        }
        else if (param_name == "latitude")
        {
            configuration->latitude = value.toFloat();
            LOGD("Latitude: " + String(configuration->latitude, 4));
        }
        else if (param_name == "longitude")
        {
            configuration->longitude = value.toFloat();
            LOGD("Longitude: " + String(configuration->longitude, 4));
        }
        else if (param_name == "refresh_interval")
        {
            configuration->refreshIntervalMinutes = value.toInt();
            LOGD("Refresh interval: " + String(configuration->refreshIntervalMinutes) + " min");
        }
        else if (param_name == "language")
        {
            configuration->language = value;
            
            // Set localization
            if (value == "en") Localization::setLanguage(LANG_EN);
            else if (value == "de") Localization::setLanguage(LANG_DE);
            else if (value == "cs") Localization::setLanguage(LANG_CS);
            else if (value == "pl") Localization::setLanguage(LANG_PL);
            else Localization::setLanguage(LANG_CS);
            
            LOGD("Language set to: " + value);
        }
    }
    
    configuration->save();
    
    String html = String((const char *)HTML_SUCCESS);
    html.replace("{{VERSION}}", String(VERSION_NUMBER));
    webServer.send(200, "text/html", html);
    setupCompleted = true;
}

void WeatherWebSetupServer::handlePOSTUpdateFirmware()
{
    String message;
    if (!Update.hasError())
    {
        message = "Update successful. Device will reboot.";
    }
    else
    {
        StreamString str;
        Update.printError(str);
        message = String("Update FAILED! ") + String(str.c_str());
    }
    LOGD(message);
    webServer.send(200, "text/html", message);
    if (!Update.hasError())
    {
        ESP.restart();
    }
}

void WeatherWebSetupServer::handleUploadFirmware()
{
    HTTPUpload &upload = webServer.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        if (!Update.begin())
        {
            LOGD("Update begin failed.");
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            LOGD("Firmware upload file write failed...");
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (Update.end(true))
        {
            LOGD("Update successful... Rebooting...");
        }
        else
        {
            LOGD("Firmware upload ending FAILED...");
        }
    }
    else
    {
        LOGD("Update Failed Unexpectedly: status=" + String(upload.status));
    }
}

void WeatherWebSetupServer::handleCSSStyle()
{
    webServer.send_P(200, "text/css", (const char *)CSS_STYLE, CSS_STYLE_END - CSS_STYLE);
}

void WeatherWebSetupServer::handleGETSetup()
{
    LOGD("handleGETSetup called");
    
    String html = String((const char *)HTML_INDEX);
    html.replace("{{VERSION}}", String(VERSION_NUMBER));
    
    webServer.send(200, "text/html", html);
    
    LOGD("handleGETSetup completed");
}

void WeatherWebSetupServer::loop()
{
    webServer.handleClient();
}

void WeatherWebSetupServer::handleWiFiScan()
{
    DynamicJsonDocument *doc = new DynamicJsonDocument(2048);
    if (!doc) {
        webServer.send(500, "text/plain", "Memory allocation failed");
        return;
    }

    int found = WiFi.scanNetworks();
    
    struct NetworkInfo {
        String ssid;
        int percent;
        bool current;
    };
    NetworkInfo networks[32];
    int networkCount = 0;
    
    for (int i = 0; i < found && networkCount < 32; i++)
    {
        String ssid = WiFi.SSID(i);
        if (ssid.isEmpty()) continue;
        
        int wifiPercent = min(max(2 * ((int)WiFi.RSSI(i) + 100), 0), 100);
        bool isCurrent = ssid == configuration->ssid;
        
        bool exists = false;
        for (int j = 0; j < networkCount; j++) {
            if (networks[j].ssid == ssid) {
                if (wifiPercent > networks[j].percent) {
                    networks[j].percent = wifiPercent;
                }
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            networks[networkCount].ssid = ssid;
            networks[networkCount].percent = wifiPercent;
            networks[networkCount].current = isCurrent;
            networkCount++;
        }
    }
    
    // Sort by signal strength
    for (int i = 0; i < networkCount - 1; i++) {
        for (int j = i + 1; j < networkCount; j++) {
            if (networks[j].percent > networks[i].percent) {
                NetworkInfo temp = networks[i];
                networks[i] = networks[j];
                networks[j] = temp;
            }
        }
    }
    
    for (int i = 0; i < networkCount; i++) {
        JsonObject item = doc->createNestedObject();
        item["ssid"] = networks[i].ssid;
        item["percent"] = networks[i].percent;
        item["current"] = networks[i].current;
    }

    String response;
    response.reserve(doc->memoryUsage() + 100);
    serializeJson(*doc, response);
    delete doc;
    webServer.send(200, "application/json", response);
}

String WeatherWebSetupServer::getWebServerSetupURL()
{
    if (WiFi.getMode() == WIFI_AP)
    {
        return "http://" + WiFi.softAPIP().toString();
    }
    else
    {
        return "http://" + WiFi.localIP().toString();
    }
}

void WeatherWebSetupServer::handleGETConfig()
{
    DynamicJsonDocument *doc = new DynamicJsonDocument(2048);
    if (!doc) {
        webServer.send(500, "text/plain", "Memory allocation failed");
        return;
    }

    (*doc)["wifiPassword"] = String(configuration->wifiPassword);   
    (*doc)["timezoneCode"] = configuration->timezone;
    (*doc)["locationName"] = configuration->locationName;
    (*doc)["latitude"] = configuration->latitude;
    (*doc)["longitude"] = configuration->longitude;
    (*doc)["refreshInterval"] = configuration->refreshIntervalMinutes;
    (*doc)["language"] = configuration->language;
    
    String response;
    response.reserve(doc->memoryUsage() + 100);
    serializeJson(*doc, response);
    delete doc;
    webServer.send(200, "application/json", response);
}

bool WeatherWebSetupServer::isSetupCompleted()
{
    return setupCompleted;
}

// EmbeddedFileHandler implementation
EmbeddedFileHandler::EmbeddedFileHandler(String uri, const uint8_t *data_start, const uint8_t *data_end, String contentType)
{
    _uri = uri;
    _data_start = data_start;
    _data_end = data_end;
    _contentType = contentType;
}

bool EmbeddedFileHandler::canHandle(HTTPMethod method, String uri)
{
    return (method == HTTP_GET) && (uri == _uri);
}

bool EmbeddedFileHandler::handle(WebServer &server, HTTPMethod requestMethod, String requestUri)
{
    size_t size = _data_end - _data_start;
    server.send_P(200, _contentType.c_str(), (const char *)_data_start, size);
    return true;
}
