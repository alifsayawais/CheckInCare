#include "NotificationManager.h"

NotificationManager::NotificationManager(const String& email, const String& phone)
    : email(email), phone(phone) {}

void NotificationManager::sendEmail(const char* smtpHost, uint16_t smtpPort, 
                                    const char* senderEmail, const char* senderPassword, 
                                    const char* recipientEmail, const char* subject, 
                                    const char* content) {
    SMTPSession smtp;
    SMTP_Message message;

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
    message.addRecipient("Recipient Name", recipientEmail);
    message.text.content = content;
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    // Connect to the SMTP server and send the email
    if (!smtp.connect(&session)) {
        Serial.println("Error connecting to SMTP server: " + smtp.errorReason());
        return;
    }

    if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("Error sending Email: " + smtp.errorReason());
    } else {
        Serial.println("Email sent successfully!");
    }

    // Clear message to free memory
    message.clear();
    smtp.closeSession();
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