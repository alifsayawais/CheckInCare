#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>

class NotificationManager {
public:
    NotificationManager(const String& email, const String& phone);
    void setSIM800Serial(HardwareSerial* sim800Serial); // Add method to set SIM800 serial
    void setAPN(const String& apn, const String& username = "", const String& password = ""); // Set custom APN
    void setSIMPIN(const String& pin); // Set SIM PIN
    void sendEmail(const char* smtpHost, uint16_t smtpPort, 
                   const char* senderEmail, const char* senderPassword, 
                   const char* subject, const char* content, 
                   int configRedLED); // Removed recipientEmail
    void sendSMS_SIM800(const String& message);
    bool sendSMS(const String& accountSID, 
        const String& authToken, 
        const String& fromNumber, 
        const String& toNumber, 
        const String& body
    );
    bool sendTwilioSMS(const String& toNumber, const String& message); // Convenience method with preset credentials

private:
    String email;
    String phone;
    HardwareSerial* sim800; // Pointer to SIM800 serial
    String apnName;
    String apnUsername;
    String apnPassword;
    String simPIN;

    bool sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout = 5000);
};

#endif // NOTIFICATION_MANAGER_H