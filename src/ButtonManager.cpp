#include "ButtonManager.h"
#include "NotificationManager.h"
#include "TimeUtils.h"

ButtonManager::ButtonManager(int buttonPin, int redPin, int bluePin, int whitePin, int configBluePin, int configGreenPin, int configRedPin, WiFiManager* wifiManager, NotificationManager* notificationManager)
    : buttonPin(buttonPin), redPin(redPin), bluePin(bluePin), whitePin(whitePin), configBluePin(configBluePin), configGreenPin(configGreenPin), configRedPin(configRedPin),
      lastPressTime(0), pressInterval(12 * 60 * 60 * 1000), debounceDelay(50), lastDebounceTime(0), lastButtonState(HIGH), buttonState(HIGH), apMode(false), wifiManager(wifiManager), connectivityModeStarted(false), wifiConnected(false), vacationModeStarted(false), consecutivePressCount(0), lastPressCheckTime(0), emailSentForSolidRed(false) {
    buttonPressed = false; // Initialize the button pressed flag
    targetTime = "";
    today = "";
    alarmSkipDate = "";
    currentState = "default";
}

String ButtonManager::getTodayDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "";
    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    return String(dateStr);
}

void ButtonManager::begin() {
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(redPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    pinMode(whitePin, OUTPUT);
    pinMode(configBluePin, OUTPUT);
    pinMode(configGreenPin, OUTPUT);
    pinMode(configRedPin, OUTPUT);

    // Ensure all LEDs are off initially
    setLED(whitePin, false);
    setLED(bluePin, false);
    setLED(redPin, false);
    setLED(configBluePin, false, true);
    setLED(configGreenPin, false, true);
    setLED(configRedPin, false, true);

    // Check for previous configuration and attempt to connect to WiFi
    if (!wifiManager->getSSID().isEmpty()) {
        Serial.println("Previous configuration found. Attempting to connect to WiFi.");
        tryConnectWiFi();
    } else {
        Serial.println("No previous configuration found. Configuration is needed.");
        indicateConfigurationNeeded();
    }
}

void ButtonManager::update() {
    checkButton();
    handleButtonState();

    if (apMode) {
        flashConfigBlueLED();
    }

    if (!wifiConnected && !apMode) {
        flashConfigRedLED();
    }

    // Reset the press count if the interval exceeds 400 ms
    if (millis() - lastPressCheckTime >= 400) {
        resetConsecutivePressCount();
    }
}

void ButtonManager::handleClient() {
    wifiManager->handleClient();

    // Check if configuration is done
    if (wifiManager->isConfigured() && !wifiConnected) {
        apMode = false;
        setLED(configBluePin, false, true); // Turn off the flashing blue LED (active LOW)
        Serial.println("Configuration completed. Attempting to connect to WiFi.");

        // Try to connect to WiFi
        tryConnectWiFi();
    }
}

bool ButtonManager::isButtonPressed() {
    bool wasPressed = buttonPressed;
    buttonPressed = false; // Reset the flag after reading
    return wasPressed;
}

void ButtonManager::checkButton() {
    bool reading = digitalRead(buttonPin);

    // Debounce logic
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) { // Button pressed
                Serial.println("Button pressed");
                lastPressTime = millis();
                connectivityModeStarted = false; // Reset flag
                vacationModeStarted = false; // Reset flag for vacation mode
                buttonPressed = true; // Set button pressed flag

                // Check for consecutive presses within 0.4 seconds
                if (millis() - lastPressCheckTime < 400) {
                    consecutivePressCount++;
                } else {
                    consecutivePressCount = 1;
                }
                lastPressCheckTime = millis();

                // If 4 consecutive presses are detected, send the email
                if (consecutivePressCount >= 4) 
                {
                    Serial.println("4 consecutive button presses detected. Sending email...");
                    String emailBody = wifiManager->getEmailBody();
                    if (emailBody.isEmpty()) 
                    {
                        emailBody = "The button was pressed four times consecutively."; // Default
                    }
                    // emailBody +=  "\nPressed At: Current Time"; // Implement time function
                    // Call the email-sending function here
                    notificationManager->sendEmail(
                        "smtp.gmail.com", 465, 
                        wifiManager->getSenderEmail().c_str(), 
                        wifiManager->getSenderPassword().c_str(), 
                        "Button Press Alert", 
                        emailBody.c_str(),
                        configRedPin );                
                    
                    // Automatically release the button and reset the press count
                    buttonState = HIGH; // Simulate button release
                    consecutivePressCount = 0; // Reset the press count
                }
            } else if (buttonState == HIGH && (millis() - lastPressTime < 3000) && !connectivityModeStarted && !vacationModeStarted) { // Button released before 3 seconds
                Serial.println("Button released before 3 seconds");
                resetTimer();
                setButtonState("white"); // Single press action
            } else if (buttonState == HIGH && (millis() - lastPressTime >= 3000) && (millis() - lastPressTime < 10000) && !vacationModeStarted && !connectivityModeStarted) { // Button released after exactly 3 seconds
                Serial.println("Button released after exactly 3 seconds");
                startVacationMode();
                resetTimer();  // Ensure the timer is reset even during vacation mode
                vacationModeStarted = true; // Set flag
                connectivityModeStarted = false; // Ensure connectivity mode is not started
            }
        }
    }

    if (buttonState == LOW && (millis() - lastPressTime >= 10000) && !connectivityModeStarted) { // Button pressed for 10 seconds
        Serial.println("Button pressed for 10 seconds");
        startConnectivityMode();
        resetTimer();  // Ensure the timer is reset even during connectivity mode
        connectivityModeStarted = true; // Set flag
        vacationModeStarted = false; // Ensure vacation mode is not started
        buttonPressed = true; // Set button pressed flag
    }

    lastButtonState = reading;
}

void ButtonManager::resetTimer() {
    lastPressTime = millis();
}

void ButtonManager::setNotificationManager(NotificationManager* notificationManager) {
    this->notificationManager = notificationManager;
}

void ButtonManager::setButtonState(String state) 
{
    if (state == currentState) return; // Avoid redundant updates

    // Log the state change
    Serial.println("Button state changed to: " + state);

    if (state == "blue") {
        digitalWrite(bluePin, HIGH);
        digitalWrite(redPin, LOW);
        digitalWrite(whitePin, LOW);
    } else if (state == "flashingRed") {
        digitalWrite(bluePin, LOW);
        digitalWrite(redPin, millis() % 1000 < 500); // Flashing effect
        digitalWrite(whitePin, LOW);
    } else if (state == "solidRed") {
        digitalWrite(bluePin, LOW);
        digitalWrite(redPin, HIGH);
        digitalWrite(whitePin, LOW);
    }
    else if (state == "white")
    {
        alarmSkipDate = getTodayDate();
        Serial.println("Alarm skipped for today: " + alarmSkipDate);
        digitalWrite(bluePin, LOW);
        digitalWrite(redPin, LOW);
        digitalWrite(whitePin, HIGH);
    } 
    currentState = state; // Update the current state

}

bool ButtonManager::isTimeWithinRange(const String& currentTime, const String& targetTime, int rangeInSeconds) {
    int currentHour, currentMinute, targetHour, targetMinute;
    sscanf(currentTime.c_str(), "%d:%d", &currentHour, &currentMinute); // Parse current time
    sscanf(targetTime.c_str(), "%d:%d", &targetHour, &targetMinute);   // Parse target time

    time_t now = currentHour * 3600 + currentMinute * 60;
    time_t target = targetHour * 3600 + targetMinute * 60;

    double diff = difftime(target, now);
    return diff <= rangeInSeconds && diff > 0;
}

void ButtonManager::setTargetTime(const String& time) {
    targetTime = time;
}

void ButtonManager::handleButtonState() 
{
    if (targetTime == "") return; // No target time set
    today = getTodayDate();
    
    if (alarmSkipDate == today)
    {
        setButtonState("white");
        return;
    }
    // Get the current time (hour and minute)
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    char currentTime[6];
    strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);

    // Check and update button state based on time
    if (isTimeWithinRange(String(currentTime), targetTime, 3600)) { // 1 hour (3600 seconds)
        setButtonState("blue");
    } else if (isTimeWithinRange(String(currentTime), targetTime, 1800)) { // 30 minutes (1800 seconds)
        setButtonState("flashingRed");
    } else if (String(currentTime) == targetTime) { // Exact target time
        setButtonState("solidRed");
    }
}

// void ButtonManager::handleButtonState() {
//     unsigned long currentTime = millis();
//     unsigned long elapsedTime = currentTime - lastPressTime;

//     if (elapsedTime >= pressInterval) {
//         setButtonState("solid red");
        
//         // Check if email has already been sent for this state
//         if (!emailSentForSolidRed) {
//             // Send email
//             Serial.println("Sending email for solid red state...");
//             String emailBody = "The button has been in the solid red state.";
//             // emailBody += "\nTriggered At: Current Time"; // Use TimeUtils to get the current time
//             notificationManager->sendEmail(
//                 "smtp.gmail.com", 465, 
//                 wifiManager->getSenderEmail().c_str(), 
//                 wifiManager->getSenderPassword().c_str(), 
//                 "Solid Red Alert", 
//                 emailBody.c_str(),
//                 configRedPin
//             );

//             // Mark email as sent
//             emailSentForSolidRed = true;
//         }
//     } else if (elapsedTime >= 10 * 60 * 60 * 1000) {
//         setButtonState("flashing red");
//     } else if (elapsedTime >= 8 * 60 * 60 * 1000) {
//         setButtonState("blue");
//     }
// }

void ButtonManager::setLED(int pin, bool state, bool activeLow) {
    if (activeLow) {
        digitalWrite(pin, state ? LOW : HIGH); // Active LOW
    } else {
        digitalWrite(pin, state ? HIGH : LOW); // Active HIGH
    }
}

void ButtonManager::flashRedLED() {
    // Implement flashing red logic
    static unsigned long lastFlashTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= 500) { // Toggle every 500ms
        digitalWrite(redPin, !digitalRead(redPin));
        lastFlashTime = currentTime;
    }
}

void ButtonManager::flashConfigBlueLED() {
    // Implement flashing blue logic for configuration mode
    static unsigned long lastFlashTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= 500) { // Toggle every 500ms
        digitalWrite(configBluePin, !digitalRead(configBluePin));
        lastFlashTime = currentTime;
    }
}

void ButtonManager::flashConfigRedLED() {
    // Implement flashing red logic when WiFi connection fails
    static unsigned long lastFlashTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= 500) { // Toggle every 500ms
        digitalWrite(configRedPin, !digitalRead(configRedPin));
        lastFlashTime = currentTime;
    }
}

void ButtonManager::blinkConfigGreenLED() {
    // Blink the green LED five times rapidly
    for (int i = 0; i < 10; i++) { // Blink 5 times (10 toggles)
        setLED(configGreenPin, true, true); // Turn on the green LED (active LOW)
        delay(100); // Wait for 100ms
        setLED(configGreenPin, false, true); // Turn off the green LED (active LOW)
        delay(100); // Wait for 100ms
    }
}

void ButtonManager::startConnectivityMode() {
    apMode = true;
    wifiManager->eraseConfig(); // Erase previous configuration
    wifiManager->begin();
    setLED(configBluePin, true, true); // Start flashing blue LED (active LOW)
    setLED(configRedPin, false, true); // Turn off the red LED (active LOW)
    wifiConnected = false; // Reset WiFi connection status
    Serial.println("Connectivity mode started. Access point is up.");
}

void ButtonManager::startVacationMode() {
    setLED(bluePin, true); // Turn on the blue LED to indicate vacation mode
    Serial.println("Vacation mode started. Device is in sleep mode.");
    // Add any additional logic for vacation/sleep mode here
}

void ButtonManager::tryConnectWiFi() {
    String ssid = wifiManager->getSSID();
    String password = wifiManager->getPassword();
    const int maxAttempts = 2;
    int attempt = 0;

    Serial.println("Connecting to WiFi...");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);

    while (attempt < maxAttempts) 
    {
        WiFi.begin(ssid.c_str(), password.c_str());
        unsigned long startAttemptTime = millis();

        // Indicate retry attempt with flashing blue LED rapidly
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) 
        {
            setLED(configBluePin, !digitalRead(configBluePin), true); // Toggle blue LED rapidly
            delay(100);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi connected.");
            wifiConnected = true;
            blinkConfigGreenLED(); // Blink the green LED five times rapidly to indicate successful connection
            setLED(configBluePin, false, true); // Ensure the blue LED is turned off
            return;
        } 
        else 
        {
            Serial.println("WiFi connection failed. Retrying...");
            attempt++;
        }
    }

    // If all attempts fail, indicate failure and switch to configuration mode
    Serial.println("WiFi connection failed after maximum attempts.");
    setLED(configRedPin, true, true); // Turn on the red LED (active LOW)
    wifiConnected = false;

    // Automatically enter configuration mode after failure
    startConnectivityMode();
}

void ButtonManager::indicateConfigurationNeeded() {
    // Indicate that configuration is needed with a solid red LED
    setLED(configRedPin, true, true); // Turn on the red LED (active LOW)
    Serial.println("Configuration is needed.");
}

int ButtonManager::getConsecutivePressCount() {
    return consecutivePressCount;
}

void ButtonManager::resetConsecutivePressCount() {
    consecutivePressCount = 0;
}