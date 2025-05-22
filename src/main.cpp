#include <Arduino.h>
#include "WiFiManager.h"
#include "ButtonManager.h"
#include "NotificationManager.h"
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SD.h>
#include <FS.h>
#include <TimeUtils.h>

#define MAIN_CODE     1
#define AUDIO_CHECK   0



#if MAIN_CODE == 1
const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "123456789";
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
#endif



#if AUDIO_CHECK == 1

// Pin definitions for SD card module (SPI)
#define SD_MISO 11
#define SD_MOSI 13
#define SD_SCK 14
#define SD_CS 12

// Audio output pin (PWM)
#define AUDIO_PIN 39

// Audio buffer
#define BUFFER_SIZE 512
uint8_t buffer[BUFFER_SIZE];

// Global variables
File audioFile;
bool playing = false;

void setup() {
  Serial.begin(115200);

  // Initialize PWM on GPIO 39
  ledcSetup(0, 44100, 8); // Channel 0, 44.1kHz frequency, 8-bit resolution
  ledcAttachPin(AUDIO_PIN, 0);

  // Configure SPI pins for SD card
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed! Check your connections.");
    return;
  }
  Serial.println("SD card initialized.");

  // Open the audio file
  audioFile = SD.open("/iphone.raw");
  if (!audioFile) {
    Serial.println("Failed to open audio file! Ensure audio.raw exists on the SD card.");
    return;
  }
  playing = true;
}

void loop() {
  if (playing && audioFile) {
    size_t bytesRead = audioFile.read(buffer, BUFFER_SIZE);
    if (bytesRead == 0) {
      // End of file
      playing = false;
      audioFile.close();
      Serial.println("Audio playback finished.");
      return;
    }

    // Write audio data to PWM
    for (size_t i = 0; i < bytesRead; i++) {
      ledcWrite(0, buffer[i]); // Output audio sample
      delayMicroseconds(23);  // Adjust for 44.1kHz playback speed
    }
  }
}
#endif
