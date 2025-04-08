#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include <WiFiManager.h>

class ButtonManager {
public:
    ButtonManager(int buttonPin, int redPin, int bluePin, int whitePin, int configBluePin, int configGreenPin, int configRedPin, WiFiManager* wifiManager);
    void begin();
    void update();
    void handleClient();
    bool isButtonPressed(); // Add this method

private:
    int buttonPin;
    int redPin;
    int bluePin;
    int whitePin;
    int configBluePin;
    int configGreenPin;
    int configRedPin;
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
    WiFiManager* wifiManager;

    void checkButton();
    void resetTimer();
    void setButtonState(const char* state);
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
    bool buttonPressed; // Add this member variable
};

#endif