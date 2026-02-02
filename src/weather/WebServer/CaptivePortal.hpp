#include <Arduino.h>
#include <DNSServer.h>

class CaptivePortal
{
public:
    void start();
    void stop();
    void loop();
    String getWiFiName();
    String getWiFiQR();
    bool anyClientConnected();
private:
    DNSServer dnsServer;
};
