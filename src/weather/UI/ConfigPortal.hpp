#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "../Storage/WeatherConfiguration.hpp"
#include "../Logging/Logging.hpp"

// Captive portal configuration
#define AP_SSID "WeatherStation-Setup"
#define AP_PASSWORD ""  // Open network for easy setup
#define DNS_PORT 53

class ConfigPortal
{
public:
    ConfigPortal() : server(80), dnsServer(), configured(false), portalActive(false) {}
    
    // Start configuration portal (Access Point mode)
    void start(WeatherConfiguration& config)
    {
        LOGD("Starting configuration portal");
        
        // Start AP
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        delay(100);
        
        apIP = WiFi.softAPIP();
        LOGD("AP IP address: " + apIP.toString());
        
        // Start DNS server (captive portal)
        dnsServer.start(DNS_PORT, "*", apIP);
        
        // Setup web server routes
        setupRoutes(config);
        server.begin();
        
        portalActive = true;
        configured = false;
        
        LOGD("Configuration portal started");
    }
    
    // Handle client requests (call in loop)
    void handle()
    {
        if (portalActive)
        {
            dnsServer.processNextRequest();
            server.handleClient();
        }
    }
    
    // Stop portal
    void stop()
    {
        if (portalActive)
        {
            server.stop();
            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            portalActive = false;
            LOGD("Configuration portal stopped");
        }
    }
    
    // Check if configuration was submitted
    bool isConfigured() { return configured; }
    
    // Check if portal is running
    bool isActive() { return portalActive; }
    
    // Get AP IP for display
    String getAPIP() { return apIP.toString(); }
    String getSSID() { return AP_SSID; }

private:
    WebServer server;
    DNSServer dnsServer;
    IPAddress apIP;
    bool configured;
    bool portalActive;
    WeatherConfiguration* configPtr;
    
    void setupRoutes(WeatherConfiguration& config)
    {
        configPtr = &config;
        
        // Main configuration page
        server.on("/", HTTP_GET, [this]() {
            server.send(200, "text/html", getConfigPage());
        });
        
        // Handle form submission
        server.on("/save", HTTP_POST, [this]() {
            handleSave();
        });
        
        // Captive portal detection endpoints
        server.on("/generate_204", HTTP_GET, [this]() {  // Android
            server.sendHeader("Location", "http://" + apIP.toString() + "/", true);
            server.send(302, "text/plain", "");
        });
        server.on("/fwlink", HTTP_GET, [this]() {  // Windows
            server.sendHeader("Location", "http://" + apIP.toString() + "/", true);
            server.send(302, "text/plain", "");
        });
        server.on("/hotspot-detect.html", HTTP_GET, [this]() {  // Apple
            server.sendHeader("Location", "http://" + apIP.toString() + "/", true);
            server.send(302, "text/plain", "");
        });
        
        // Catch all
        server.onNotFound([this]() {
            server.sendHeader("Location", "http://" + apIP.toString() + "/", true);
            server.send(302, "text/plain", "");
        });
    }
    
    void handleSave()
    {
        if (configPtr == nullptr)
        {
            server.send(500, "text/html", "Error: Configuration not available");
            return;
        }
        
        // Get form values
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        String location = server.arg("location");
        String lat = server.arg("latitude");
        String lon = server.arg("longitude");
        String refresh = server.arg("refresh");
        
        LOGD("Received configuration:");
        LOGD("SSID: " + ssid);
        LOGD("Location: " + location);
        LOGD("Lat: " + lat + ", Lon: " + lon);
        
        // Validate
        if (ssid.length() == 0)
        {
            server.send(400, "text/html", getErrorPage("WiFi SSID je povinné"));
            return;
        }
        
        if (lat.length() == 0 || lon.length() == 0)
        {
            server.send(400, "text/html", getErrorPage("Souřadnice lokace jsou povinné"));
            return;
        }
        
        // Update configuration
        configPtr->ssid = ssid;
        configPtr->wifiPassword = password;
        configPtr->latitude = lat.toFloat();
        configPtr->longitude = lon.toFloat();
        configPtr->locationName = location.length() > 0 ? location : "Moje lokace";
        
        if (refresh.length() > 0)
        {
            int refreshMin = refresh.toInt();
            if (refreshMin >= 5 && refreshMin <= 60)
            {
                configPtr->refreshIntervalMinutes = refreshMin;
            }
        }
        
        // Save to flash
        configPtr->save();
        configured = true;
        
        LOGD("Configuration saved");
        
        // Send success response
        server.send(200, "text/html", getSuccessPage());
    }
    
    String getConfigPage()
    {
        String html = R"(
<!DOCTYPE html>
<html lang="cs">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weather Station - Nastavení</title>
    <style>
        * { box-sizing: border-box; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; }
        body { margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 500px; margin: 0 auto; background: white; border-radius: 12px; padding: 30px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; margin-top: 0; font-size: 24px; }
        h2 { color: #666; font-size: 18px; margin-top: 25px; border-bottom: 1px solid #eee; padding-bottom: 10px; }
        label { display: block; margin-top: 15px; color: #333; font-weight: 500; }
        input[type="text"], input[type="password"], input[type="number"] { 
            width: 100%; padding: 12px; margin-top: 5px; border: 1px solid #ddd; 
            border-radius: 8px; font-size: 16px; 
        }
        input:focus { outline: none; border-color: #007aff; }
        .row { display: flex; gap: 15px; }
        .row > div { flex: 1; }
        button { 
            width: 100%; padding: 15px; margin-top: 25px; 
            background: #007aff; color: white; border: none; 
            border-radius: 8px; font-size: 18px; cursor: pointer;
            font-weight: 600;
        }
        button:hover { background: #0056b3; }
        .hint { color: #888; font-size: 12px; margin-top: 5px; }
        .location-hint { background: #f0f7ff; padding: 15px; border-radius: 8px; margin-top: 10px; font-size: 13px; }
        .location-hint a { color: #007aff; }
    </style>
</head>
<body>
    <div class="container">
        <h1>⛅ Weather Station</h1>
        <p>Nastavte WiFi připojení a vaši lokaci pro předpověď počasí.</p>
        
        <form action="/save" method="POST">
            <h2>📶 WiFi připojení</h2>
            <label>Název sítě (SSID)
                <input type="text" name="ssid" placeholder="Vaše WiFi síť" required>
            </label>
            <label>Heslo
                <input type="password" name="password" placeholder="WiFi heslo">
            </label>
            
            <h2>📍 Lokace</h2>
            <label>Název místa
                <input type="text" name="location" placeholder="např. Praha, Brno, Můj dům" value="Praha">
            </label>
            <div class="row">
                <div>
                    <label>Zeměpisná šířka
                        <input type="text" name="latitude" placeholder="50.0755" value="50.0755" required>
                    </label>
                </div>
                <div>
                    <label>Zeměpisná délka
                        <input type="text" name="longitude" placeholder="14.4378" value="14.4378" required>
                    </label>
                </div>
            </div>
            <div class="location-hint">
                💡 <strong>Tip:</strong> Souřadnice zjistíte na 
                <a href="https://www.google.com/maps" target="_blank">Google Maps</a> - 
                klikněte pravým tlačítkem na místo a vyberte souřadnice.
            </div>
            
            <h2>⚙️ Další nastavení</h2>
            <label>Interval obnovení (minuty)
                <input type="number" name="refresh" min="5" max="60" value="10">
            </label>
            <p class="hint">Doporučeno 10-15 minut pro úsporu baterie.</p>
            
            <button type="submit">💾 Uložit nastavení</button>
        </form>
    </div>
</body>
</html>
)";
        return html;
    }
    
    String getSuccessPage()
    {
        String html = R"(
<!DOCTYPE html>
<html lang="cs">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Nastavení uloženo</title>
    <style>
        body { font-family: -apple-system, sans-serif; display: flex; justify-content: center; 
               align-items: center; height: 100vh; margin: 0; background: #f5f5f5; }
        .container { text-align: center; background: white; padding: 40px; border-radius: 12px; 
                     box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .icon { font-size: 60px; }
        h1 { color: #28a745; }
        p { color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <div class="icon">✅</div>
        <h1>Nastavení uloženo!</h1>
        <p>Weather Station se nyní restartuje a připojí k vaší WiFi síti.</p>
        <p>Můžete toto okno zavřít.</p>
    </div>
</body>
</html>
)";
        return html;
    }
    
    String getErrorPage(const String& error)
    {
        String html = R"(
<!DOCTYPE html>
<html lang="cs">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Chyba</title>
    <style>
        body { font-family: -apple-system, sans-serif; display: flex; justify-content: center; 
               align-items: center; height: 100vh; margin: 0; background: #f5f5f5; }
        .container { text-align: center; background: white; padding: 40px; border-radius: 12px; }
        .icon { font-size: 60px; }
        h1 { color: #dc3545; }
        a { color: #007aff; }
    </style>
</head>
<body>
    <div class="container">
        <div class="icon">❌</div>
        <h1>Chyba</h1>
        <p>)" + error + R"(</p>
        <p><a href="/">Zpět na nastavení</a></p>
    </div>
</body>
</html>
)";
        return html;
    }
};
