#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "WifiManager.h"
#include "ButtonManager.h"
#include "NotificationManager.h"
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SD.h>
#include <FS.h>
#include <TimeUtils.h>


const int userGreenLED = 7;
const int mainButton = 48;
const int mainLED = 38;
const int configRedLED = 4;
const int configGreenLED = 5;
const int configBlueLED = 6;

// WS2812B LED configuration using Adafruit NeoPixel
#define NUM_LEDS 4
#define LED_DATA_PIN mainLED

Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);


const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "123456789";

NotificationManager* notificationManager = nullptr;
WiFiManager wifiManager(ap_ssid, ap_password);
ButtonManager buttonManager(mainButton, configBlueLED, configGreenLED, configRedLED, &wifiManager, notificationManager);

// WS2812B LED control functions using NeoPixel
void setMainLEDs(uint32_t color) {
    for(int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void setMainLEDsRed() {
    setMainLEDs(strip.Color(255, 0, 0));
}

void setMainLEDsBlue() {
    setMainLEDs(strip.Color(0, 0, 255));
}

void setMainLEDsWhite() {
    setMainLEDs(strip.Color(255, 255, 255));
}

void setMainLEDsOff() {
    setMainLEDs(strip.Color(0, 0, 0));
}

void setup() 
{
    Serial.begin(115200);
    
    // Initialize WS2812B LEDs on GPIO 31
    strip.begin();
    strip.setBrightness(50); // Set brightness to 50% (0-255)
    setMainLEDsOff(); // Start with LEDs off

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
    buttonManager.setNotificationManager(notificationManager);
    // Set the time zone
    String timeZone = wifiManager.getTimeZone();
    configureTimeZone(timeZone);

    // Log the current time
    Serial.println("Current Time: " + getCurrentTime());

    String userTime = wifiManager.getButtonTime(); // Replace with actual user input
    buttonManager.setTargetTime(userTime);
    Serial.println("Target Time Set: " + userTime);
}

void loop() 
{
    buttonManager.handleClient();
    buttonManager.update();
    notificationManager->update();
}
