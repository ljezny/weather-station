#include "WiFiManager.hpp"
#include <esp_wifi.h>

// RTC memory for WiFi cache - survives deep sleep
RTC_DATA_ATTR WiFiCache_t wifiCacheData = {0};
RTC_DATA_ATTR uint8_t wifiConnectionAttemptsData = 0;

WiFiManager::WiFiManager() : cache(&wifiCacheData), connectionAttempts(wifiConnectionAttemptsData), lastBeginTime(0)
{
}

void WiFiManager::begin()
{
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.setMinSecurity(WIFI_AUTH_OPEN);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
}

int WiFiManager::rssiToPercent(int rssi)
{
    return min(max(2 * (rssi + 100), 0), 100);
}

bool WiFiManager::hasCachedConnection() const
{
    return cache->bssid[0] != 0 && cache->channel != 0;
}

void WiFiManager::clearCache()
{
    memset(cache, 0, sizeof(WiFiCache_t));
    resetToDHCP();
    LOGD("WiFi cache cleared");
}

void WiFiManager::resetConnectionAttempts()
{
    connectionAttempts = 0;
    wifiConnectionAttemptsData = 0;
}

bool WiFiManager::isLeaseValid() const
{
    if (cache->ip == 0) return false;
    if (cache->leaseStartTime == 0) {
        LOGD("Lease start time unknown, assuming valid");
        return true;
    }
    
    time_t now = time(NULL);
    if (now < 1000000000) {
        LOGD("Current time not set, assuming lease is valid");
        return true;
    }
    
    uint32_t leaseTime = cache->leaseTime > 0 ? cache->leaseTime : DEFAULT_DHCP_LEASE_SECONDS;
    time_t safeExpiry = cache->leaseStartTime + (leaseTime * 9 / 10);
    bool valid = now < safeExpiry;
    
    LOGD("Lease check: start=" + String((long)cache->leaseStartTime) + " duration=" + String(leaseTime) + "s now=" + String((long)now) + " valid=" + String(valid ? "yes" : "no"));
    return valid;
}

bool WiFiManager::applyCachedIP()
{
    if (cache->ip == 0) {
        LOGD("No cached IP available");
        return false;
    }
    
    if (!isLeaseValid()) {
        LOGD("Cached IP lease expired, will use DHCP");
        cache->ip = 0;
        cache->leaseStartTime = 0;
        return false;
    }
    
    LOGD("Applying cached static IP: " + IPAddress(cache->ip).toString());
    WiFi.config(IPAddress(cache->ip), IPAddress(cache->gateway), IPAddress(cache->subnet), IPAddress(cache->dns));
    return true;
}

void WiFiManager::resetToDHCP()
{
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
}

bool WiFiManager::hasCachedCredentials() const
{
    return cache->ssid[0] != 0;
}

void WiFiManager::saveToCache(const String& ssid, const String& password)
{
    memcpy(cache->bssid, WiFi.BSSID(), sizeof(cache->bssid));
    cache->channel = WiFi.channel();
    
    strncpy(cache->ssid, ssid.c_str(), WIFI_SSID_MAX_LEN - 1);
    cache->ssid[WIFI_SSID_MAX_LEN - 1] = 0;
    strncpy(cache->password, password.c_str(), WIFI_PASS_MAX_LEN - 1);
    cache->password[WIFI_PASS_MAX_LEN - 1] = 0;
    
    uint32_t currentIP = (uint32_t)WiFi.localIP();
    if (cache->ip != currentIP) {
        cache->ip = currentIP;
        cache->gateway = (uint32_t)WiFi.gatewayIP();
        cache->subnet = (uint32_t)WiFi.subnetMask();
        cache->dns = (uint32_t)WiFi.dnsIP();
        
        time_t now = time(NULL);
        if (now > 1000000000) {
            cache->leaseStartTime = now;
            LOGD("Cached new DHCP info - IP: " + WiFi.localIP().toString());
        } else {
            cache->leaseStartTime = 0;
        }
        cache->leaseTime = 0;
    }
}

void WiFiManager::updateLeaseStartTime()
{
    if (cache->ip != 0 && cache->leaseStartTime == 0) {
        time_t now = time(NULL);
        if (now > 1000000000) {
            cache->leaseStartTime = now;
            LOGD("Updated lease start time to: " + String((long)cache->leaseStartTime));
        }
    }
}

bool WiFiManager::findBestAP(const String& ssid, uint8_t* outBSSID, uint8_t* outChannel, int* outRSSI)
{
    int found = WiFi.scanNetworks();
    int bestRSSI = -1000;
    int bestIndex = -1;
    
    for (int i = 0; i < found; i++) {
        if (WiFi.SSID(i) == ssid) {
            LOGD("Found AP: " + ssid + " BSSID: " + WiFi.BSSIDstr(i) + " RSSI: " + String(WiFi.RSSI(i)));
            if (WiFi.RSSI(i) > bestRSSI) {
                bestRSSI = WiFi.RSSI(i);
                bestIndex = i;
            }
        }
    }
    
    if (bestIndex >= 0) {
        memcpy(outBSSID, WiFi.BSSID(bestIndex), 6);
        *outChannel = WiFi.channel(bestIndex);
        *outRSSI = bestRSSI;
        LOGD("Best AP selected: " + ssid + " RSSI: " + String(bestRSSI));
        return true;
    }
    
    return false;
}

void WiFiManager::startCachedConnection()
{
    if (!hasCachedCredentials() || !hasCachedConnection()) {
        LOGD("Cannot start cached connection - missing data");
        return;
    }
    
    lastBeginTime = millis();
    LOGD("Ultra-fast WiFi reconnect using cached data");
    applyCachedIP();
    WiFi.begin(cache->ssid, cache->password, cache->channel, cache->bssid);
}

void WiFiManager::startConnection(const String& ssid, const String& password)
{
    lastBeginTime = millis();
    
    strncpy(cache->ssid, ssid.c_str(), WIFI_SSID_MAX_LEN - 1);
    cache->ssid[WIFI_SSID_MAX_LEN - 1] = 0;
    strncpy(cache->password, password.c_str(), WIFI_PASS_MAX_LEN - 1);
    cache->password[WIFI_PASS_MAX_LEN - 1] = 0;
    
    if (hasCachedConnection()) {
        LOGD("Fast WiFi reconnect - BSSID cached");
        applyCachedIP();
        WiFi.begin(ssid.c_str(), password.c_str(), cache->channel, cache->bssid);
    } else {
        LOGD("Connecting to WiFi: " + ssid);
        
        uint8_t bssid[6];
        uint8_t channel;
        int rssi;
        
        if (findBestAP(ssid, bssid, &channel, &rssi)) {
            memcpy(cache->bssid, bssid, 6);
            cache->channel = channel;
            WiFi.begin(ssid.c_str(), password.c_str(), channel, bssid);
        } else {
            LOGD("No matching SSID found, trying direct connect.");
            WiFi.begin(ssid.c_str(), password.c_str());
        }
    }
}

WiFiConnectionResult_t WiFiManager::updateConnection(const String& ssid, const String& password)
{
    wl_status_t status = WiFi.status();
    
    switch (status) {
    case WL_CONNECTED:
        if (WiFi.localIP() != INADDR_NONE) {
            connectionAttempts = 0;
            wifiConnectionAttemptsData = 0;
            saveToCache(ssid, password);
            LOGD("Connected to WiFi. IP: " + WiFi.localIP().toString());
            return WIFI_CONN_SUCCESS;
        }
        break;
        
    case WL_NO_SSID_AVAIL:
    case WL_CONNECT_FAILED:
    case WL_DISCONNECTED:
        {
            unsigned long now = millis();
            if (lastBeginTime > 0 && (now - lastBeginTime) < 5000) {
                return WIFI_CONN_IN_PROGRESS;
            }
            
            connectionAttempts++;
            wifiConnectionAttemptsData = connectionAttempts;
            LOGD("WiFi connection attempt " + String(connectionAttempts) + " failed.");
            
            if (connectionAttempts == 1) {
                LOGD("Clearing WiFi cache");
                clearCache();
            }
            
            if (connectionAttempts < MAX_WIFI_CONNECTION_ATTEMPTS) {
                LOGD("Retrying WiFi connection...");
                WiFi.disconnect();
                delay(500);
                
                uint8_t bssid[6];
                uint8_t channel;
                int rssi;
                
                if (findBestAP(ssid, bssid, &channel, &rssi)) {
                    memcpy(cache->bssid, bssid, 6);
                    cache->channel = channel;
                    WiFi.begin(ssid.c_str(), password.c_str(), channel, bssid);
                } else {
                    WiFi.begin(ssid.c_str(), password.c_str());
                }
                
                lastBeginTime = millis();
                return WIFI_CONN_FAILED_RETRY;
            } else {
                LOGD("WiFi connection failed after all attempts.");
                connectionAttempts = 0;
                wifiConnectionAttemptsData = 0;
                lastBeginTime = 0;
                return WIFI_CONN_FAILED_GIVE_UP;
            }
        }
        break;
        
    default:
        break;
    }
    
    return WIFI_CONN_IN_PROGRESS;
}

std::vector<WiFiNetworkInfo_t> WiFiManager::scanNetworks(const String& currentSSID)
{
    std::vector<WiFiNetworkInfo_t> networks;
    
    int found = WiFi.scanNetworks();
    
    for (int i = 0; i < found; i++) {
        WiFiNetworkInfo_t info;
        info.ssid = WiFi.SSID(i);
        info.bssid = WiFi.BSSIDstr(i);
        info.rssi = WiFi.RSSI(i);
        info.channel = WiFi.channel(i);
        info.signalPercent = rssiToPercent(info.rssi);
        info.isCurrentNetwork = (info.ssid == currentSSID);
        networks.push_back(info);
    }
    
    std::sort(networks.begin(), networks.end(), [](const WiFiNetworkInfo_t& a, const WiFiNetworkInfo_t& b) {
        return a.rssi > b.rssi;
    });
    
    return networks;
}
