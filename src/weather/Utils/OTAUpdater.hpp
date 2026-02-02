#ifndef OTAUpdater_hpp
#define OTAUpdater_hpp

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class OTAUpdater
{ 
public:
    OTAUpdater();
    ~OTAUpdater();
    bool check();
    void update();
private:
    WiFiClientSecure *client;
    String targetMd5;
};
#endif
