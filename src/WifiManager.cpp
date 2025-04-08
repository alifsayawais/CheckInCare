#include "WiFiManager.h"
#include <nvs.h>
#include <nvs_flash.h>
#include <Preferences.h>

Preferences preferences;

WiFiManager::WiFiManager(const char* ap_ssid, const char* ap_password)
    : ap_ssid(ap_ssid), ap_password(ap_password), server(80), configured(false) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        // Retry nvs_flash_init
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    loadConfigFromNVS(); // Load configuration from NVS
}

void WiFiManager::begin() {
    WiFi.softAP(ap_ssid, ap_password);
    server.on("/", HTTP_GET, std::bind(&WiFiManager::handleRoot, this));
    server.on("/config", HTTP_POST, std::bind(&WiFiManager::handleConfig, this));
    server.onNotFound(std::bind(&WiFiManager::handleNotFound, this));
    server.begin();
    Serial.println("HTTP server started");
}

void WiFiManager::handleClient() {
    server.handleClient();
}

bool WiFiManager::isConfigured() {
    return configured;
}

void WiFiManager::handleRoot() {
    server.send(200, "text/html", "<form action=\"/config\" method=\"POST\">"
                                  "WiFi SSID: <input type=\"text\" name=\"ssid\"><br>"
                                  "WiFi Password: <input type=\"text\" name=\"password\"><br>"
                                  "Email: <input type=\"text\" name=\"email\"><br>"
                                  "Phone: <input type=\"text\" name=\"phone\"><br>"
                                  "<input type=\"submit\" value=\"Submit\"></form>");
}

void WiFiManager::handleConfig() {
    ssid = server.arg("ssid");
    password = server.arg("password");
    email = server.arg("email");
    phone = server.arg("phone");

    // Store the configuration in NVS
    saveConfigToNVS();

    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    Serial.println("Email: " + email);
    Serial.println("Phone: " + phone);
    
    configured = true;
    server.send(200, "text/plain", "Configuration saved. You can close this window.");
}

void WiFiManager::handleNotFound() {
    server.send(404, "text/plain", "404: Not found");
}

void WiFiManager::eraseConfig() {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    Serial.println("Previous configuration erased.");
    configured = false;
    ssid = "";
    password = "";
    email = "";
    phone = "";
}

String WiFiManager::getSSID() {
    return ssid;
}

String WiFiManager::getPassword() {
    return password;
}

String WiFiManager::getEmail() {
    return email;
}

String WiFiManager::getPhone() {
    return phone;
}

void WiFiManager::saveConfigToNVS() {
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("email", email);
    preferences.putString("phone", phone);
    preferences.end();
}

void WiFiManager::loadConfigFromNVS() {
    preferences.begin("wifi-config", true);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    email = preferences.getString("email", "");
    phone = preferences.getString("phone", "");
    preferences.end();

    // Check if configuration is valid
    if (ssid.length() > 0 && password.length() > 0) {
        configured = true;
        Serial.println("Configuration loaded from NVS.");
    } else {
        configured = false;
        Serial.println("No valid configuration found in NVS.");
    }
}