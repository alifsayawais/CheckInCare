#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <nvs.h>
#include <nvs_flash.h>

class WiFiManager {
public:
    WiFiManager(const char* ap_ssid, const char* ap_password);
    void begin();
    void handleClient();
    bool isConfigured();
    String getEmail();
    String getPhone();
    String getEmailBody(); 
    String getEmailSubject();

    // Add sender credential getters
    String getSenderEmail();
    String getSenderPassword();

    void eraseConfig();
    String getSSID();
    String getPassword();
    String getButtonTime();
    String getTimeZone();
    String getAuthToken();
    String getWarningThreshold();
    void reloadConfigFromNVS(); // Force reload configuration from NVS

private:
    const char* ap_ssid;
    const char* ap_password;
    WebServer server;
    bool configured;
    String ssid;
    String password;
    String email;
    String phone;
    String emailBody;
    String emailSubject;
    String buttonTime;
    String timeZone;
    String authToken;
    String warningThreshold;

    void handleRoot();
    void handleNotFound();
    void handleConfig();
    void saveConfigToNVS(); // New method to save config to NVS
    void loadConfigFromNVS(); // New method to load config from NVS

    String senderEmail;
    String senderPassword;
};

#endif // WIFI_MANAGER_H