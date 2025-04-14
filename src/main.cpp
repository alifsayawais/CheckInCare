#include <Arduino.h>
#include "WiFiManager.h"
#include "ButtonManager.h"
#include "NotificationManager.h"
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "12345678";

const int redPin = 10;
const int bluePin = 47;
const int whitePin = 48;
const int configBluePin = 16;
const int configGreenPin = 1;
const int configRedPin = 17;

const int buttonPin = 35;

NotificationManager* notificationManager = nullptr;
WiFiManager wifiManager(ap_ssid, ap_password);
ButtonManager buttonManager(buttonPin, redPin, bluePin, whitePin, configBluePin, configGreenPin, configRedPin, &wifiManager, notificationManager);

// WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP, "pool.ntp.org");

// String getCurrentTime() {
//     timeClient.update();
//     return timeClient.getFormattedTime(); // Returns "HH:MM:SS"
// }

int buttonPressCount = 0;

void setup() 
{
    Serial.begin(115200);

    if (!EEPROM.begin(512)) {
        Serial.println("Failed to initialize EEPROM. Formatting NVS...");
        if (nvs_flash_erase() == ESP_OK) {
            Serial.println("NVS formatted successfully. Reinitializing EEPROM...");
            if (!EEPROM.begin(512)) {
                Serial.println("Failed to reinitialize EEPROM after formatting NVS.");
                return;
            }
        } else {
            Serial.println("Failed to format NVS.");
            return;
        }
    }

    // timeClient.begin();
    // timeClient.setTimeOffset(0); // Adjust for your timezone (e.g., 3600 for UTC+1)
    buttonManager.begin();

    String email = wifiManager.getEmail();
    String phone = wifiManager.getPhone();
    notificationManager = new NotificationManager(email, phone);
    buttonManager.setNotificationManager(notificationManager);
}

void loop() 
{
    buttonManager.handleClient();
    buttonManager.update();
    notificationManager->update();
}
