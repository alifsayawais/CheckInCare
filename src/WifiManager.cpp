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
    server.send(200, "text/html",
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>Configuration Form</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; background-color: #f7f7f7; margin: 0; padding: 0; }"
        ".container { max-width: 600px; margin: 50px auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }"
        "h1 { text-align: center; color: #333; margin-bottom: 20px; }"
        "form { display: flex; flex-direction: column; gap: 15px; }"
        ".form-group { display: flex; flex-direction: column; }"
        "label { font-weight: bold; margin-bottom: 5px; }"
        "input, textarea { padding: 10px; border: 1px solid #ccc; border-radius: 4px; font-size: 16px; width: 100%; box-sizing: border-box; }"
        "textarea { resize: vertical; }"
        "button { padding: 10px 15px; background-color: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }"
        "button:hover { background-color: #0056b3; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"container\">"
        "<h1>WiFi Configuration</h1>"
        "<form action=\"/config\" method=\"POST\">"
        "<div class=\"form-group\">"
        "<label for=\"ssid\">WiFi SSID</label>"
        "<input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"Enter WiFi SSID\" required>"
        "</div>"
        "<div class=\"form-group\">"
        "<label for=\"password\">WiFi Password</label>"
        "<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Enter WiFi Password\" required>"
        "</div>"
        "<div class=\"form-group\">"
        "<label for=\"email\">Recipient's Email</label>"
        "<input type=\"email\" id=\"email\" name=\"email\" placeholder=\"Enter Recipient's Email\" required>"
        "</div>"
        "<div class=\"form-group\">"
        "<label for=\"phone\">Phone</label>"
        "<input type=\"text\" id=\"phone\" name=\"phone\" placeholder=\"Enter Phone Number\">"
        "</div>"
        "<div class=\"form-group\">"
        "<label for=\"sender_email\">Sender Email</label>"
        "<input type=\"email\" id=\"sender_email\" name=\"sender_email\" placeholder=\"Enter Sender Email\" required>"
        "</div>"
        "<div class=\"form-group\">"
        "<label for=\"sender_password\">Sender Password</label>"
        "<input type=\"password\" id=\"sender_password\" name=\"sender_password\" placeholder=\"Enter Sender Password\" required>"
        "</div>"
        "<div class=\"form-group\">"
        "<label for=\"email_body\">Email Body</label>"
        "<textarea id=\"email_body\" name=\"email_body\" rows=\"4\" placeholder=\"Enter Email Body\"></textarea>"
        "</div>"
        "<button type=\"submit\">Submit</button>"
        "</form>"
        "</div>"
        "</body>"
        "</html>");
}




void WiFiManager::handleConfig() {
    ssid = server.arg("ssid");
    password = server.arg("password");
    email = server.arg("email");
    phone = server.arg("phone");
    senderEmail = server.arg("sender_email");
    senderPassword = server.arg("sender_password");
    emailBody = server.arg("email_body");

    // Store the configuration in NVS
    saveConfigToNVS();

    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    Serial.println("Email: " + email);
    Serial.println("Phone: " + phone);
    
    configured = true;
    server.send(200, "text/plain", "Configuration saved. You can close this window.");
}


// Add getters
String WiFiManager::getSenderEmail() { return senderEmail; }
String WiFiManager::getSenderPassword() { return senderPassword; }
String WiFiManager::getEmailBody() {  return emailBody;   }

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
    preferences.putString("sender_email", senderEmail);
    preferences.putString("sender_password", senderPassword);
    preferences.putString("email_body", emailBody);
    preferences.end();
}

void WiFiManager::loadConfigFromNVS() {
    preferences.begin("wifi-config", true);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    email = preferences.getString("email", "");
    phone = preferences.getString("phone", "");
    senderEmail = preferences.getString("sender_email", "");
    senderPassword = preferences.getString("sender_password", "");
    emailBody = preferences.getString("email_body", "");
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