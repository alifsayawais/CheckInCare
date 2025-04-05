#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <Arduino.h>

class NotificationManager {
public:
    NotificationManager(const String& email, const String& phone);
    void sendEmail(const String& subject, const String& body);
    void sendSMS(const String& message);

private:
    String email;
    String phone;
    void setupSIM800();
};

#endif // NOTIFICATION_MANAGER_H