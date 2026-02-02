#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "../Storage/WeatherConfiguration.hpp"
#include "../Logging/Logging.hpp"

extern const uint8_t HTML_INDEX[] asm("_binary_src_weather_WebServer_html_index_html_start");
extern const uint8_t HTML_INDEX_END[] asm("_binary_src_weather_WebServer_html_index_html_end");
extern const uint8_t HTML_SUCCESS[] asm("_binary_src_weather_WebServer_html_success_html_start");
extern const uint8_t HTML_SUCCESS_END[] asm("_binary_src_weather_WebServer_html_success_html_end");
extern const uint8_t CSS_STYLE[] asm("_binary_src_weather_WebServer_html_style_css_start");
extern const uint8_t CSS_STYLE_END[] asm("_binary_src_weather_WebServer_html_style_css_end");
extern const uint8_t SVG_LOGO[] asm("_binary_src_weather_WebServer_html_logo_svg_start");
extern const uint8_t SVG_LOGO_END[] asm("_binary_src_weather_WebServer_html_logo_svg_end");
extern const uint8_t LOCALIZATION_EN[] asm("_binary_src_weather_WebServer_html_localization_en_json_start");
extern const uint8_t LOCALIZATION_EN_END[] asm("_binary_src_weather_WebServer_html_localization_en_json_end");
extern const uint8_t LOCALIZATION_DE[] asm("_binary_src_weather_WebServer_html_localization_de_json_start");
extern const uint8_t LOCALIZATION_DE_END[] asm("_binary_src_weather_WebServer_html_localization_de_json_end");
extern const uint8_t LOCALIZATION_CS[] asm("_binary_src_weather_WebServer_html_localization_cs_json_start");
extern const uint8_t LOCALIZATION_CS_END[] asm("_binary_src_weather_WebServer_html_localization_cs_json_end");
extern const uint8_t LOCALIZATION_PL[] asm("_binary_src_weather_WebServer_html_localization_pl_json_start");
extern const uint8_t LOCALIZATION_PL_END[] asm("_binary_src_weather_WebServer_html_localization_pl_json_end");

class WeatherWebSetupServer
{
public:
    WeatherWebSetupServer(WeatherConfiguration *configuration);
    void begin();
    void end();
    void loop();
    bool isSetupCompleted();
    String getWebServerSetupURL();

private:
    WebServer webServer;
    WeatherConfiguration *configuration;
    bool setupCompleted = false;
    
    void handleGETSetup();
    void handleCSSStyle();
    void handleWiFiScan();
    void handleGETConfig();
    void handlePOSTSetup();
    void handlePOSTUpdateFirmware();
    void handleUploadFirmware();
};

class EmbeddedFileHandler : public RequestHandler
{
public:
    EmbeddedFileHandler(String uri, const uint8_t *data_start, const uint8_t *data_end, String contentType);
    bool canHandle(HTTPMethod method, String uri) override;
    bool handle(WebServer &server, HTTPMethod requestMethod, String requestUri) override;

protected:
    String _uri;
    const uint8_t *_data_start;
    const uint8_t *_data_end;
    String _contentType;
};
