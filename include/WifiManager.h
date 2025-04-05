#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WiFiManager {
public:
    WiFiManager(const char* ap_ssid, const char* ap_password);
    void begin();
    void handleClient();
    bool isConfigured();
    String getEmail();
    String getPhone();
    void eraseConfig();
    String getSSID();
    String getPassword();

private:
    const char* ap_ssid;
    const char* ap_password;
    WebServer server;
    bool configured;
    String ssid;
    String password;
    String email;
    String phone;

    void handleRoot();
    void handleNotFound();
    void handleConfig();
};

#endif // WIFI_MANAGER_H