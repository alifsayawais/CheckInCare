#include <Arduino.h>
#include "WiFiManager.h"
#include "ButtonManager.h"
#include "NotificationManager.h"

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

void setup() {
    Serial.begin(115200); // Initialize serial communication
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
}