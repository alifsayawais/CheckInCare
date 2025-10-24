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
    void sendEmail(const char* smtpHost, uint16_t smtpPort, 
                   const char* senderEmail, const char* senderPassword, 
                   const char* subject, const char* content, 
                   int configRedLED); // Removed recipientEmail
    void sendSMS_SIM800(const String& message);
    void update();
    bool sendSMS(const String& accountSID, 
        const String& authToken, 
        const String& fromNumber, 
        const String& toNumber, 
        const String& body
    );

private:
    unsigned long blinkStartTime = 0;
    bool isBlinking = false;
    int blinkPin = -1;
    
    String email;
    String phone;

    void setupSIM800();
};

#endif // NOTIFICATION_MANAGER_H