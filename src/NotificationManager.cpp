#include "NotificationManager.h"
#include "WiFiManager.h" // Include WiFiManager to access getEmail()

extern WiFiManager wifiManager; // Reference the global instance of WiFiManager

NotificationManager::NotificationManager(const String& email, const String& phone)
    : email(email), phone(phone) {}

void NotificationManager::sendEmail(const char* smtpHost, uint16_t smtpPort, 
                                    const char* senderEmail, const char* senderPassword, 
                                    const char* subject, const char* content, 
                                    int configRedPin) { // Removed recipientEmail
    SMTPSession smtp;
    SMTP_Message message;

    // Retrieve recipient email dynamically
    String recipientEmail = wifiManager.getEmail();

    if (recipientEmail.isEmpty()) {
        Serial.println("Recipient email is empty. Cannot send email.");
        return;
    }

    // Configure configRedPin for LED blinking
    pinMode(configRedPin, OUTPUT);

    unsigned long blinkStart = millis();
    unsigned long blinkDuration = 5000;
    bool ledState = false;

    Serial.println("Blinking LED before sending email...");
    while (millis() - blinkStart < blinkDuration) {
        if (millis() % 200 < 100) {
            ledState = !ledState;
            digitalWrite(configRedPin, ledState ? LOW : HIGH);
        }
    }

    digitalWrite(configRedPin, HIGH);

    // Configure SMTP server
    ESP_Mail_Session session;
    session.server.host_name = smtpHost;
    session.server.port = smtpPort;
    session.login.email = senderEmail;
    session.login.password = senderPassword;
    session.login.user_domain = "";

    // Set up the email content
    message.sender.name = "ESP32 Sender";
    message.sender.email = senderEmail;
    message.subject = subject;
    message.addRecipient("Recipient Name", recipientEmail.c_str());
    message.text.content = content;
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&session)) {
        Serial.println("Error connecting to SMTP server: " + smtp.errorReason());
        return;
    }

    if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("Error sending Email: " + smtp.errorReason());
    } else {
        Serial.println("Email sent successfully!");
    }

    message.clear();
    smtp.closeSession();
}

void NotificationManager::sendSMS(const String& message) {
    setupSIM800();
    Serial.println("Sending SMS:");
    Serial.println("Message: " + message);
}

void NotificationManager::setupSIM800() {
    Serial.println("Setting up SIM800...");
}