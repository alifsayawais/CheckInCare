#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>

class NotificationManager {
public:
    NotificationManager(const String& email, const String& phone);
    void sendEmail(const char* smtpHost, uint16_t smtpPort, 
                   const char* senderEmail, const char* senderPassword, 
                   const char* subject, const char* content, 
                   int configRedPin); // Removed recipientEmail
    void sendSMS(const String& message);

private:
    String email;
    String phone;

    void setupSIM800();
};

#endif // NOTIFICATION_MANAGER_H