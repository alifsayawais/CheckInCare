#include "NotificationManager.h"

NotificationManager::NotificationManager(const String& email, const String& phone)
    : email(email), phone(phone) {}

void NotificationManager::sendEmail(const String& subject, const String& body) {
    // Implement email sending logic using the specified email address
    Serial.println("Sending email:");
    Serial.println("Subject: " + subject);
    Serial.println("Body: " + body);
}

void NotificationManager::sendSMS(const String& message) {
    // Implement SMS sending logic using the specified phone number and SIM800
    setupSIM800();
    Serial.println("Sending SMS:");
    Serial.println("Message: " + message);
}

void NotificationManager::setupSIM800() {
    // Implement SIM800 setup logic here
    Serial.println("Setting up SIM800...");
}