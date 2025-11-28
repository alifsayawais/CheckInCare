#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <NotificationManager.h> // Include NotificationManager

// External LED control functions for WS2812B
extern void setMainLEDsRed();
extern void setMainLEDsBlue();
extern void setMainLEDsWhite();
extern void setMainLEDsOff();

// External sound functions
extern void playReminderSound();
extern void playUrgentSound();
extern void playTargetTimeSound();

class ButtonManager {
public:
    ButtonManager(int mainButton, int configBlueLED, int configGreenLED, int configRedLED,
    WiFiManager* wifiManager, NotificationManager* notificationManager);

    void begin();
    void update();
    void handleClient();
    bool isButtonPressed(); // Add this method
    int getConsecutivePressCount(); // Add this method
    void resetConsecutivePressCount(); // Add this method
    void setNotificationManager(NotificationManager* notificationManager);
    void setTargetTime(const String& time); // Set user-provided time
    String getTodayDate();
    void printTimeLeftToAlarm();

private:
    
    String targetTime;
    String currentState; 
    String today;
    String alarmSkipDate;

    int mainButton;
    int configBlueLED;
    int configGreenLED;
    int configRedLED;

    unsigned long lastPressTime;
    unsigned long pressInterval;
    unsigned long debounceDelay;
    unsigned long lastDebounceTime;
    
    bool lastButtonState;
    bool buttonState;
    bool apMode;
    bool connectivityModeStarted;
    bool wifiConnected;
    bool vacationModeStarted;
    bool validPressStarted; // Track if button was pressed from released state
    

    WiFiManager* wifiManager;
    NotificationManager* notificationManager;

    int consecutivePressCount; 
    unsigned long lastPressCheckTime; 

    void checkButton();
    void resetTimer();
    void setButtonState(String state);
    void handleButtonState();
    void setLED(int pin, bool state, bool activeLow = false);
    void flashRedLED();
    void flashRapidRedLED();       // New function for 20-minute warning
    void flashConfigBlueLED();
    void flashConfigRedLED();
    void blinkConfigGreenLED();
    void startConnectivityMode();
    void startVacationMode();
    void tryConnectWiFi();
    void indicateConfigurationNeeded();
    bool isTimeWithinRange(const String& currentTime, const String& targetTime, int rangeInSeconds);
    bool buttonPressed;
    bool emailSentForSolidRed;
    bool messageSent;
    bool twentyMinWarningShown;    // Track if 20-minute warning has been shown
    bool missedButtonEmailSent;   // Track if missed button email has been sent
    bool deviceJustStarted;       // Track if device just powered on
    unsigned long startupTime;    // Time when device was powered on
    unsigned long connectivityModeStartTime; // Time when connectivity mode was started

};

#endif