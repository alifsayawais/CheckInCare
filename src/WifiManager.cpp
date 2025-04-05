#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* ap_ssid, const char* ap_password)
    : ap_ssid(ap_ssid), ap_password(ap_password), server(80), configured(false) {}

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

    // Store the configuration (in a real application, you should save it to non-volatile storage)
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
    // Erase the previous configuration (in a real application, you should erase it from non-volatile storage)
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