#include <Arduino.h>
#include "WiFiManager.h"
#include "ButtonManager.h"
#include "NotificationManager.h"
#include <EEPROM.h>

const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "12345678";

const int redPin = 10;
const int bluePin = 47;
const int whitePin = 48;
const int configBluePin = 16;
const int configGreenPin = 1;
const int configRedPin = 17;

const int buttonPin = 35;

WiFiManager wifiManager(ap_ssid, ap_password);
ButtonManager buttonManager(buttonPin, redPin, bluePin, whitePin, configBluePin, configGreenPin, configRedPin, &wifiManager);
NotificationManager* notificationManager;

int buttonPressCount = 0;

void setup() {
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

    buttonManager.begin();

    String email = wifiManager.getEmail();
    String phone = wifiManager.getPhone();
    notificationManager = new NotificationManager(email, phone);
}

void loop() {
    buttonManager.handleClient();
    buttonManager.update();

    if (buttonManager.isButtonPressed()) {
        buttonPressCount++;
        Serial.println("Button pressed: " + String(buttonPressCount));

        if (buttonManager.getConsecutivePressCount() >= 4) {
            Serial.println("Button pressed four times consecutively. Sending email...");
            notificationManager->sendEmail("smtp.gmail.com", 465,
                                           "awais013pk@gmail.com", "xgdo uadb ffbu gbdf",
                                           "Button Press Alert",
                                           "The button was pressed four times consecutively.", configRedPin);
            buttonPressCount = 0;
            buttonManager.resetConsecutivePressCount();
        }
    }
}