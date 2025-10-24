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

};

#endif