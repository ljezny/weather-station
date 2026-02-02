#include "CaptivePortal.hpp"
#include <WiFi.h>

void CaptivePortal::start()
{
    WiFi.softAP(getWiFiName().c_str());
    dnsServer.start(53, "*", WiFi.softAPIP());
}

void CaptivePortal::stop()
{
    dnsServer.stop();
    WiFi.softAPdisconnect();
}

void CaptivePortal::loop()
{
    dnsServer.processNextRequest();
}

String CaptivePortal::getWiFiName()
{
    char ssid[23];
    snprintf(ssid, 23, "%llX", ESP.getEfuseMac());
    return "WEATHER-" + String(ssid);
}

String CaptivePortal::getWiFiQR()
{
    String qrCode;
    qrCode += "WIFI:S:" + getWiFiName() + ";";
    qrCode += ";";
    return qrCode;
}

bool CaptivePortal::anyClientConnected() {
    return WiFi.softAPgetStationNum() > 0;
}
