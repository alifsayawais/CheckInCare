#include <Arduino.h>
#include "WiFiManager.h"
#include "ButtonManager.h"
#include "NotificationManager.h"
#include <EEPROM.h> // Include the EEPROM library

// Define the SSID and password for the Access Point
const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "12345678";

// Define GPIO pins for the LEDs
const int redPin = 10;
const int bluePin = 47;
const int whitePin = 48;
const int configBluePin = 16; // Updated to GPIO 16 (active LOW)
const int configGreenPin = 1; // Updated to GPIO 1 (active LOW)
const int configRedPin = 17;  // Updated to GPIO 17 (active LOW)

// Define GPIO pin for the button
const int buttonPin = 35; // Updated to GPIO 35

// Create instances of the managers
WiFiManager wifiManager(ap_ssid, ap_password);
ButtonManager buttonManager(buttonPin, redPin, bluePin, whitePin, configBluePin, configGreenPin, configRedPin, &wifiManager);
NotificationManager* notificationManager;

int buttonPressCount = 0; // Counter for button presses

void setup() {
    Serial.begin(115200); // Initialize serial communication

    // Format NVS if initialization fails
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

    // Initialize the managers
    buttonManager.begin();

    // Retrieve notification details from WiFiManager
    String email = wifiManager.getEmail();
    String phone = wifiManager.getPhone();
    notificationManager = new NotificationManager(email, phone);
}

void loop() {
    // Handle client requests
    buttonManager.handleClient();

    // Update button manager
    buttonManager.update();

    // Check for button press and increment counter
    if (buttonManager.isButtonPressed()) {
        buttonPressCount++;
        Serial.println("Button pressed: " + String(buttonPressCount));

        // Check for four consecutive button presses
        if (buttonManager.getConsecutivePressCount() >= 4) {
            Serial.println("Button pressed four times consecutively. Sending email...");
            notificationManager->sendEmail("smtp.gmail.com", 465, // Use SMTP port 465 for SSL
                                           "awais013pk@gmail.com", "xgdo uadb ffbu gbdf", 
                                           "awais12pk@gmail.com",
                                           "Button Press Alert", 
                                           "The button was pressed four times consecutively.", configRedPin);
            buttonPressCount = 0; // Reset counter after sending email
            buttonManager.resetConsecutivePressCount(); // Reset consecutive press count
        }
    }
}