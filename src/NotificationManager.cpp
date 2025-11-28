#include "NotificationManager.h"
#include "WiFiManager.h"
#include <ESP_Mail_Client.h>

extern WiFiManager wifiManager;

NotificationManager::NotificationManager(const String& email, const String& phone)
    : email(email), phone(phone), sim800(nullptr), apnName("internet"), apnUsername(""), apnPassword(""), simPIN("") {
}

void NotificationManager::setSIM800Serial(HardwareSerial* sim800Serial) {
    sim800 = sim800Serial;
}

void NotificationManager::setAPN(const String& apn, const String& username, const String& password) {
    apnName = apn;
    apnUsername = username;
    apnPassword = password;
}

void NotificationManager::setSIMPIN(const String& pin) {
    simPIN = pin;
}

void NotificationManager::sendEmail(const char* smtpHost, uint16_t smtpPort, 
                                    const char* senderEmail, const char* senderPassword, 
                                    const char* subject, const char* content, 
                                    int configRedLED) {
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected - cannot send email");
        return;
    }
    
    Serial.println("Sending email...");
    
    // Create SMTP session
    SMTPSession smtp;
    
    // Declare the session config data
    ESP_Mail_Session session;
    
    // Set the session config
    session.server.host_name = smtpHost;
    session.server.port = smtpPort;
    session.login.email = senderEmail;
    session.login.password = senderPassword;
    session.login.user_domain = "";
    
    // Declare the message class
    SMTP_Message message;
    
    // Set the message headers
    message.sender.name = "Patient Care Device";
    message.sender.email = senderEmail;
    message.subject = subject;
    message.addRecipient("Caregiver", email.c_str());
    
    // Set the message content
    message.text.content = content;
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_high;
    
    // Connect to server and send the email
    if (!smtp.connect(&session)) {
        Serial.print("Email connection failed: ");
        Serial.println(smtp.errorReason());
        return;
    }
    
    if (!MailClient.sendMail(&smtp, &message)) {
        Serial.print("Email send failed: ");
        Serial.println(smtp.errorReason());
    } else {
        Serial.println("Email sent successfully!");
    }
    
    smtp.closeSession();
}

void NotificationManager::sendSMS_SIM800(const String& message) {
    if (!sim800) {
        Serial.println("SIM800 serial not initialized!");
        return;
    }

    Serial.println("Sending SMS via SIM800C...");
    
    // Basic SIM800 checks
    if (!sendATCommand("AT", "OK", 5000)) {
        Serial.println("SIM800C not responding!");
        return;
    }

    if (!sendATCommand("AT+CPIN?", "READY", 5000)) {
        Serial.println("SIM not ready!");
        return;
    }

    if (!sendATCommand("AT+CMGF=1", "OK", 5000)) {
        Serial.println("Failed to set SMS text mode!");
        return;
    }
    
    // Simplified SMS sending - use stored phone number
    String cmd = "AT+CMGS=\"" + phone + "\"";
    sim800->println(cmd);
    delay(2000);
    
    if (sim800->available()) {
        String resp = sim800->readString();
        if (resp.indexOf(">") >= 0) {
            sim800->print(message);
            delay(100);
            sim800->write(26); // Ctrl+Z
            
            // Wait for confirmation
            unsigned long start = millis();
            while (millis() - start < 10000) {
                if (sim800->available()) {
                    String result = sim800->readString();
                    if (result.indexOf("+CMGS:") >= 0) {
                        Serial.println("SMS sent successfully");
                        return;
                    } else if (result.indexOf("ERROR") >= 0) {
                        Serial.println("SMS failed");
                        return;
                    }
                }
                delay(100);
            }
        }
    }
    Serial.println("SMS timeout");
}

bool NotificationManager::sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout) {
    if (!sim800) {
        return false;
    }
    
    // Clear any existing data in the buffer
    while (sim800->available()) {
        sim800->read();
    }
    
    sim800->println(command);
    
    unsigned long startTime = millis();
    String response = "";
    
    while (millis() - startTime < timeout) {
        if (sim800->available()) {
            char c = sim800->read();
            response += c;
            
            if (response.indexOf(expectedResponse) >= 0) {
                return true;
            }
            if (response.indexOf("ERROR") >= 0) {
                return false;
            }
        }
        delay(10);
    }
    
    return false;
}

bool NotificationManager::sendSMS(const String& accountSID, const String& authToken, const String& fromNumber, const String& toNumber, const String& body) {
    Serial.println("Sending SMS via Twilio...");
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected - cannot send SMS");
        return false;
    }
    
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // For simplicity - in production, use proper certificates
    
    // Twilio API endpoint
    String url = "https://api.twilio.com/2010-04-01/Accounts/" + accountSID + "/Messages.json";
    
    http.begin(client, url);
    
    // Set headers
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Create Basic Auth header
    String auth = accountSID + ":" + authToken;
    String authHeader = "Basic " + base64::encode(auth);
    http.addHeader("Authorization", authHeader);
    
    // Create POST data
    String postData = "To=" + toNumber + "&From=" + fromNumber + "&Body=" + body;
    
    // Send POST request
    int httpResponseCode = http.POST(postData);
    
    if (httpResponseCode > 0) {
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("SMS sent successfully!");
            http.end();
            return true;
        } else {
            Serial.println("SMS failed with code: " + String(httpResponseCode));
        }
    } else {
        Serial.println("HTTP request failed: " + String(httpResponseCode));
    }
    
    http.end();
    return false;
}

bool NotificationManager::sendTwilioSMS(const String& toNumber, const String& message) {
    // Your Twilio credentials
    const String accountSID = "ACf421c9e76d3e2c2914b1138c3c03b214";
    const String authToken = "b1dff8ef20bb7b47fa09613de39268ed";
    const String fromNumber = "+18449893949";
    
    return sendSMS(accountSID, authToken, fromNumber, toNumber, message);
}