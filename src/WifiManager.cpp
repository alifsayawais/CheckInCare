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
    "<html>"
    "<head>"
    "<title>WiFi Configuration</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; background-color: #f4f4f4; }"
    ".container { max-width: 500px; margin: 50px auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }"
    "h1 { text-align: center; color: #333; margin-bottom: 20px; }"
    "form { display: flex; flex-direction: column; gap: 15px; }"
    ".form-group { display: flex; flex-direction: column; }"
    ".password-container { position: relative; display: flex; align-items: center; }"
    ".password-toggle { position: absolute; right: 10px; background: none; border: none; cursor: pointer; color: #007bff; font-size: 14px; padding: 0; }"
    ".password-toggle:hover { color: #0056b3; }"
    "label { font-weight: bold; margin-bottom: 5px; }"
    "input, textarea, select { padding: 10px; border: 1px solid #ccc; border-radius: 4px; font-size: 16px; width: 100%; box-sizing: border-box; }"
    "input[type='password'], input[type='text'] { padding-right: 50px; }"
    "textarea { resize: vertical; }"
    "button { padding: 10px 15px; background-color: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }"
    "button:hover { background-color: #0056b3; }"
    "</style>"
    "<script>"
    "function togglePassword(fieldId, buttonId) {"
    "  var field = document.getElementById(fieldId);"
    "  var button = document.getElementById(buttonId);"
    "  if (field.type === 'password') {"
    "    field.type = 'text';"
    "    button.textContent = 'Hide';"
    "  } else {"
    "    field.type = 'password';"
    "    button.textContent = 'Show';"
    "  }"
    "}"
    "</script>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<h1>WiFi Configuration</h1>"
    "<form action=\"/config\" method=\"POST\">"

    "<div class=\"form-group\">"
    "<label for=\"ssid\">WiFi SSID</label>"
    "<input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"Enter WiFi SSID\" value=\"" + ssid + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"password\">WiFi Password</label>"
    "<div class=\"password-container\">"
    "<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Enter WiFi Password\" value=\"" + password + "\">"
    "<button type=\"button\" class=\"password-toggle\" id=\"toggle-password\" onclick=\"togglePassword('password', 'toggle-password')\">Show</button>"
    "</div>"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"email\">Recipient's Email</label>"
    "<input type=\"email\" id=\"email\" name=\"email\" placeholder=\"Enter Recipient's Email\" value=\"" + email + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"phone\">Phone</label>"
    "<input type=\"text\" id=\"phone\" name=\"phone\" placeholder=\"Enter Phone Number in format +xxxxxxxxxx \" value=\"" + phone + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"sender_email\">Sender Email</label>"
    "<input type=\"email\" id=\"sender_email\" name=\"sender_email\" placeholder=\"Enter Sender Email\" value=\"" + senderEmail + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"sender_password\">Sender Password</label>"
    "<div class=\"password-container\">"
    "<input type=\"password\" id=\"sender_password\" name=\"sender_password\" placeholder=\"Enter Sender Password\" value=\"" + senderPassword + "\">"
    "<button type=\"button\" class=\"password-toggle\" id=\"toggle-sender-password\" onclick=\"togglePassword('sender_password', 'toggle-sender-password')\">Show</button>"
    "</div>"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"auth_token\">Auth Token</label>"
    "<input type=\"text\" id=\"auth_token\" name=\"auth_token\" placeholder=\"Enter Twilio Auth Token\" value=\"" + authToken + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"email_subject\">Email Subject</label>"
    "<input type=\"text\" id=\"email_subject\" name=\"email_subject\" placeholder=\"Enter Email Subject\" value=\"" + emailSubject + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"email_body\">Message</label>"
    "<textarea id=\"email_body\" name=\"email_body\" rows=\"4\" placeholder=\"Enter Message Body\">" + emailBody + "</textarea>"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"button_time\">Button Press Time</label>"
    "<input type=\"time\" id=\"button_time\" name=\"button_time\" value=\"" + buttonTime + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"warning_threshold\">Warning Time (minutes before)</label>"
    "<input type=\"number\" id=\"warning_threshold\" name=\"warning_threshold\" min=\"1\" max=\"60\" placeholder=\"20\" value=\"" + warningThreshold + "\">"
    "</div>"

    "<div class=\"form-group\">"
    "<label for=\"time_zone\">Time Zone</label>"
    "<select id=\"time_zone\" name=\"time_zone\">"
    "<option value=\"UTC-12\"" + (timeZone == "UTC-12" ? " selected" : "") + ">UTC-12</option>"
    "<option value=\"UTC-11\"" + (timeZone == "UTC-11" ? " selected" : "") + ">UTC-11</option>"
    "<option value=\"UTC-10\"" + (timeZone == "UTC-10" ? " selected" : "") + ">UTC-10</option>"
    "<option value=\"UTC-9:30\"" + (timeZone == "UTC-9:30" ? " selected" : "") + ">UTC-9:30</option>"
    "<option value=\"UTC-9\"" + (timeZone == "UTC-9" ? " selected" : "") + ">UTC-9</option>"
    "<option value=\"UTC-8\"" + (timeZone == "UTC-8" ? " selected" : "") + ">UTC-8</option>"
    "<option value=\"UTC-7\"" + (timeZone == "UTC-7" ? " selected" : "") + ">UTC-7</option>"
    "<option value=\"UTC-6\"" + (timeZone == "UTC-6" ? " selected" : "") + ">UTC-6</option>"
    "<option value=\"UTC-5\"" + (timeZone == "UTC-5" ? " selected" : "") + ">UTC-5</option>"
    "<option value=\"UTC-4:30\"" + (timeZone == "UTC-4:30" ? " selected" : "") + ">UTC-4:30</option>"
    "<option value=\"UTC-4\"" + (timeZone == "UTC-4" ? " selected" : "") + ">UTC-4</option>"
    "<option value=\"UTC-3:30\"" + (timeZone == "UTC-3:30" ? " selected" : "") + ">UTC-3:30</option>"
    "<option value=\"UTC-3\"" + (timeZone == "UTC-3" ? " selected" : "") + ">UTC-3</option>"
    "<option value=\"UTC-2\"" + (timeZone == "UTC-2" ? " selected" : "") + ">UTC-2</option>"
    "<option value=\"UTC-1\"" + (timeZone == "UTC-1" ? " selected" : "") + ">UTC-1</option>"
    "<option value=\"UTC+0\"" + (timeZone == "UTC+0" ? " selected" : "") + ">UTC+0</option>"
    "<option value=\"UTC+1\"" + (timeZone == "UTC+1" ? " selected" : "") + ">UTC+1</option>"
    "<option value=\"UTC+2\"" + (timeZone == "UTC+2" ? " selected" : "") + ">UTC+2</option>"
    "<option value=\"UTC+3\"" + (timeZone == "UTC+3" ? " selected" : "") + ">UTC+3</option>"
    "<option value=\"UTC+3:30\"" + (timeZone == "UTC+3:30" ? " selected" : "") + ">UTC+3:30</option>"
    "<option value=\"UTC+4\"" + (timeZone == "UTC+4" ? " selected" : "") + ">UTC+4</option>"
    "<option value=\"UTC+4:30\"" + (timeZone == "UTC+4:30" ? " selected" : "") + ">UTC+4:30</option>"
    "<option value=\"UTC+5\"" + (timeZone == "UTC+5" ? " selected" : "") + ">UTC+5</option>"
    "<option value=\"UTC+5:30\"" + (timeZone == "UTC+5:30" ? " selected" : "") + ">UTC+5:30</option>"
    "<option value=\"UTC+5:45\"" + (timeZone == "UTC+5:45" ? " selected" : "") + ">UTC+5:45</option>"
    "<option value=\"UTC+6\"" + (timeZone == "UTC+6" ? " selected" : "") + ">UTC+6</option>"
    "<option value=\"UTC+6:30\"" + (timeZone == "UTC+6:30" ? " selected" : "") + ">UTC+6:30</option>"
    "<option value=\"UTC+7\"" + (timeZone == "UTC+7" ? " selected" : "") + ">UTC+7</option>"
    "<option value=\"UTC+8\"" + (timeZone == "UTC+8" ? " selected" : "") + ">UTC+8</option>"
    "<option value=\"UTC+8:45\"" + (timeZone == "UTC+8:45" ? " selected" : "") + ">UTC+8:45</option>"
    "<option value=\"UTC+9\"" + (timeZone == "UTC+9" ? " selected" : "") + ">UTC+9</option>"
    "<option value=\"UTC+9:30\"" + (timeZone == "UTC+9:30" ? " selected" : "") + ">UTC+9:30</option>"
    "<option value=\"UTC+10\"" + (timeZone == "UTC+10" ? " selected" : "") + ">UTC+10</option>"
    "<option value=\"UTC+10:30\"" + (timeZone == "UTC+10:30" ? " selected" : "") + ">UTC+10:30</option>"
    "<option value=\"UTC+11\"" + (timeZone == "UTC+11" ? " selected" : "") + ">UTC+11</option>"
    "<option value=\"UTC+12\"" + (timeZone == "UTC+12" ? " selected" : "") + ">UTC+12</option>"
    "<option value=\"UTC+12:45\"" + (timeZone == "UTC+12:45" ? " selected" : "") + ">UTC+12:45</option>"
    "<option value=\"UTC+13\"" + (timeZone == "UTC+13" ? " selected" : "") + ">UTC+13</option>"
    "<option value=\"UTC+14\"" + (timeZone == "UTC+14" ? " selected" : "") + ">UTC+14</option>"    
    "</select>"
    "</div>"

    "<button type=\"submit\">Save Configuration</button>"
    "</form>"
    "</div>"
    "</body>"
    "</html>");
}


void WiFiManager::handleConfig() 
{
    // Only update each field if the incoming POST value is not empty
    String incoming;

    incoming = server.arg("ssid");
    if (!incoming.isEmpty()) ssid = incoming;

    incoming = server.arg("password");
    if (!incoming.isEmpty()) password = incoming;

    incoming = server.arg("email");
    if (!incoming.isEmpty()) email = incoming;

    incoming = server.arg("phone");
    if (!incoming.isEmpty()) phone = incoming;

    incoming = server.arg("sender_email");
    if (!incoming.isEmpty()) senderEmail = incoming;

    incoming = server.arg("sender_password");
    if (!incoming.isEmpty()) senderPassword = incoming;

    incoming = server.arg("email_body");
    if (!incoming.isEmpty()) emailBody = incoming;

    incoming = server.arg("button_time");
    if (!incoming.isEmpty()) buttonTime = incoming;

    incoming = server.arg("time_zone");
    if (!incoming.isEmpty()) timeZone = incoming;

    incoming = server.arg("auth_token");
    if (!incoming.isEmpty()) authToken = incoming;

    incoming = server.arg("email_subject");
    if (!incoming.isEmpty()) emailSubject = incoming;

    incoming = server.arg("warning_threshold");
    if (!incoming.isEmpty()) warningThreshold = incoming;

    // Save merged config
    saveConfigToNVS();

    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    Serial.println("Email: " + email);
    Serial.println("Phone: " + phone);
    Serial.println("Button Time: " + buttonTime);
    Serial.println("Time Zone: " + timeZone);

    configured = true;
    server.send(200, "text/plain", "Configuration saved. You can close this window.");
    delay(1000);
    ESP.restart();
}


// Add getters
String WiFiManager::getSenderEmail() { return senderEmail; }
String WiFiManager::getSenderPassword() { return senderPassword; }
String WiFiManager::getEmailBody() {  return emailBody;   }
String WiFiManager::getEmailSubject() { return emailSubject; }

void WiFiManager::handleNotFound() {
    server.send(404, "text/plain", "404: Not found");
}

void WiFiManager::eraseConfig() 
{
    configured = false;
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

String WiFiManager::getButtonTime() { 
    return buttonTime; 
}

String WiFiManager::getTimeZone() { 
    return timeZone; 
}

String WiFiManager::getAuthToken() {
    return authToken;
}

String WiFiManager::getWarningThreshold() {
    return warningThreshold;
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
    preferences.putString("email_subject", emailSubject);
    preferences.putString("button_time", buttonTime);
    preferences.putString("time_zone", timeZone);
    preferences.putString("auth_token",authToken);
    preferences.putString("warning_threshold", warningThreshold);
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
    emailSubject = preferences.getString("email_subject", "Alert");
    buttonTime = preferences.getString("button_time", "");
    timeZone = preferences.getString("time_zone", "");
    authToken = preferences.getString("auth_token","");
    warningThreshold = preferences.getString("warning_threshold", "20");
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