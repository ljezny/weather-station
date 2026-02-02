#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <algorithm>
#include "../Logging/Logging.hpp"

#define MAX_WIFI_CONNECTION_ATTEMPTS 3

// Default DHCP lease time assumption (most routers use 24h, we use conservative 1h)
#define DEFAULT_DHCP_LEASE_SECONDS (60 * 60)  // 1 hour

// Max length for cached credentials
#define WIFI_SSID_MAX_LEN 33
#define WIFI_PASS_MAX_LEN 65

// WiFi connection cache structure - stored in RTC memory for fast reconnect after deep sleep
typedef struct __attribute__((packed)) {
    uint8_t bssid[6];
    uint8_t channel;
    uint8_t _padding;
    uint32_t ip;
    uint32_t gateway;
    uint32_t subnet;
    uint32_t dns;
    time_t leaseStartTime;
    uint32_t leaseTime;
    char ssid[WIFI_SSID_MAX_LEN];
    char password[WIFI_PASS_MAX_LEN];
} WiFiCache_t;

// Scanned WiFi network info
typedef struct {
    String ssid;
    String bssid;
    int32_t rssi;
    uint8_t channel;
    int signalPercent;
    bool isCurrentNetwork;
} WiFiNetworkInfo_t;

// Connection result
typedef enum {
    WIFI_CONN_SUCCESS,
    WIFI_CONN_IN_PROGRESS,
    WIFI_CONN_FAILED_RETRY,
    WIFI_CONN_FAILED_GIVE_UP,
} WiFiConnectionResult_t;

class WiFiManager
{
public:
    WiFiManager();
    
    void begin();
    void startConnection(const String& ssid, const String& password);
    WiFiConnectionResult_t updateConnection(const String& ssid, const String& password);
    std::vector<WiFiNetworkInfo_t> scanNetworks(const String& currentSSID = "");
    WiFiCache_t& getCache() { return *cache; }
    void clearCache();
    bool hasCachedConnection() const;
    bool hasCachedCredentials() const;
    String getCachedSSID() const { return String(cache->ssid); }
    String getCachedPassword() const { return String(cache->password); }
    void startCachedConnection();
    String getConnectedBSSID() const { return WiFi.BSSIDstr(); }
    uint8_t getConnectedChannel() const { return WiFi.channel(); }
    IPAddress getLocalIP() const { return WiFi.localIP(); }
    static int rssiToPercent(int rssi);
    void resetConnectionAttempts();
    uint8_t getConnectionAttempts() const { return connectionAttempts; }
    void updateLeaseStartTime();

private:
    WiFiCache_t* cache;
    uint8_t connectionAttempts;
    unsigned long lastBeginTime;
    
    bool findBestAP(const String& ssid, uint8_t* outBSSID, uint8_t* outChannel, int* outRSSI);
    void saveToCache(const String& ssid, const String& password);
    bool applyCachedIP();
    bool isLeaseValid() const;
    void resetToDHCP();
};
