#include "ButtonManager.h"
#include "NotificationManager.h"
#include "TimeUtils.h"

ButtonManager::ButtonManager(int mainButton, int configBlueLED, int configGreenLED, int configRedLED, WiFiManager* wifiManager, NotificationManager* notificationManager)
    : mainButton(mainButton), configBlueLED(configBlueLED), configGreenLED(configGreenLED), configRedLED(configRedLED),
      lastPressTime(0), pressInterval(12 * 60 * 60 * 1000), debounceDelay(50), lastDebounceTime(0), lastButtonState(HIGH), buttonState(HIGH), apMode(false), wifiManager(wifiManager), connectivityModeStarted(false), wifiConnected(false), vacationModeStarted(false), validPressStarted(false), consecutivePressCount(0), lastPressCheckTime(0), emailSentForSolidRed(false), messageSent(false) {
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
    pinMode(mainButton, INPUT_PULLUP);
    pinMode(configBlueLED, OUTPUT);
    pinMode(configGreenLED, OUTPUT);
    pinMode(configRedLED, OUTPUT);

    // Ensure all config LEDs are off initially
    setLED(configBlueLED, false, true);
    setLED(configGreenLED, false, true);
    setLED(configRedLED, false, true);
    
    // Initialize button state and timing to prevent false triggers at startup
    delay(100); // Allow pin to stabilize
    buttonState = digitalRead(mainButton);
    lastButtonState = buttonState;
    lastPressTime = millis(); // Set initial press time to current time
    lastDebounceTime = millis();

    // Check for previous configuration and attempt to connect to WiFi
    if (!wifiManager->getSSID().isEmpty()) {
        Serial.println("Previous configuration found. Attempting to connect to WiFi.");
        tryConnectWiFi();
    } else {
        Serial.println("No previous configuration found. Configuration is needed.");
        indicateConfigurationNeeded();
    }
}

void ButtonManager::update() 
{
    checkButton();
    handleButtonState();

    if (apMode) 
    {
        flashConfigBlueLED();
    }

    if (!wifiConnected && !apMode) 
    {
        flashConfigRedLED();
    }

    // Handle red LED flashing for flashingRed state
    if (currentState == "flashingRed") {
        flashRedLED();
    }
    // Reset the press count if the interval exceeds 400 ms
    if (millis() - lastPressCheckTime >= 400) 
    {
        resetConsecutivePressCount();
    }
}

void ButtonManager::handleClient() {
    wifiManager->handleClient();

    // Check if configuration is done
    if (wifiManager->isConfigured() && !wifiConnected) {
        apMode = false;
        setLED(configBlueLED, false, true); // Turn off the flashing blue LED (active LOW)
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
    bool reading = digitalRead(mainButton);

    // Prevent false triggering during the first 2 seconds after startup
    if (millis() < 2000) {
        lastButtonState = reading;
        buttonState = reading;
        lastPressTime = millis();
        return;
    }

    // Debounce logic
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) { // Button pressed
                Serial.println("Button pressed");
                Serial.print("Setting lastPressTime to: ");
                Serial.println(millis());
                lastPressTime = millis();
                validPressStarted = true; // Mark that button was pressed from released state
                Serial.print("validPressStarted set to: ");
                Serial.println(validPressStarted);
                connectivityModeStarted = false; // Reset flag
                // vacationModeStarted = false; // Reset flag for vacation mode
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

                    notificationManager->sendEmail(
                        "smtp.gmail.com", 465, 
                        wifiManager->getSenderEmail().c_str(), 
                        wifiManager->getSenderPassword().c_str(), 
                        wifiManager->getEmailSubject().c_str(), 
                        emailBody.c_str(),
                        configRedLED );                
                    

                    notificationManager->sendSMS(
                    "ACf421c9e76d3e2c2914b1138c3c03b214",
                    wifiManager->getAuthToken(),
                    "+18449893949",   // must be in E.164 format, e.g., "+1234567890"
                    wifiManager->getPhone(),   // must be in E.164 format, e.g., "+1987654321"
                    wifiManager->getEmailBody()   
                    );
                    
                    // Automatically release the button and reset the press count
                    buttonState = HIGH; // Simulate button release
                    consecutivePressCount = 0; // Reset the press count
                }
            } 
            else if (buttonState == HIGH && (millis() - lastPressTime < 3000) && !connectivityModeStarted) 
            {
                Serial.println("Button released before 3 seconds");
                validPressStarted = false; // Reset valid press flag on release
                resetTimer();
                Serial.print("Vacation mode flag before exit: ");
                Serial.println(vacationModeStarted);
                if (vacationModeStarted) {
                    vacationModeStarted = false;
                    Serial.println("Vacation mode exited due to short press.");
                    setMainLEDsOff(); // Turn off vacation LEDs
                }
                alarmSkipDate = getTodayDate();
                Serial.println("Alarm skipped for today: " + alarmSkipDate);
                setButtonState("white"); // Single press action (will do nothing if already white)
            }
            else if (buttonState == HIGH && (millis() - lastPressTime >= 3000) && (millis() - lastPressTime < 10000) && !vacationModeStarted && !connectivityModeStarted) { // Button released after exactly 3 seconds
                Serial.println("Button released after exactly 3 seconds");
                Serial.println("Setting vacationModeStarted to true");
                validPressStarted = false; // Reset valid press flag on release
                startVacationMode();
                resetTimer();  // Ensure the timer is reset even during vacation mode
                vacationModeStarted = true; // Set flag
                connectivityModeStarted = false; // Ensure connectivity mode is not started
            }
        }
    }

    // Debug output for long button presses
    if (buttonState == LOW && validPressStarted && (millis() - lastPressTime > 1000)) {
        static unsigned long lastDebugTime = 0;
        if (millis() - lastDebugTime > 1000) { // Print every second
            Serial.print("Button held for: ");
            Serial.print((millis() - lastPressTime) / 1000);
            Serial.print(" seconds. connectivityModeStarted: ");
            Serial.print(connectivityModeStarted);
            Serial.print(", validPressStarted: ");
            Serial.println(validPressStarted);
            lastDebugTime = millis();
        }
    }

    if (buttonState == LOW && (millis() - lastPressTime >= 10000) && !connectivityModeStarted && validPressStarted) { // Button pressed for 10 seconds
        Serial.println("Button pressed for 10 seconds");
        Serial.print("Current millis: ");
        Serial.println(millis());
        Serial.print("lastPressTime: ");
        Serial.println(lastPressTime);
        Serial.print("Time held: ");
        Serial.println(millis() - lastPressTime);
        startConnectivityMode();
        resetTimer();  // Ensure the timer is reset even during connectivity mode
        connectivityModeStarted = true; // Set flag
        vacationModeStarted = false; // Ensure vacation mode is not started
        validPressStarted = false; // Reset the valid press flag
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
    if (state == currentState) return;

    Serial.println("Button state changed to: " + state);
    if (state == "blue") 
    {
        setMainLEDsBlue();
    } 
    else if (state == "flashingRed") 
    {
        // Red flashing handled in update() method
    } 
    else if (state == "solidRed") 
    {
        setMainLEDsRed();
    }
    else if (state == "white")
    {
        setMainLEDsWhite();
    } 
    else if (state == "off")
    {
        setMainLEDsOff();
    }

    // Turn off flashing LED when leaving flashingRed
    if (currentState == "flashingRed" && state != "flashingRed") {
        setMainLEDsOff();
    }

    currentState = state;
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

    if (String(currentTime) == targetTime) 
    { // Exact target time
        setButtonState("solidRed");
        // Check if email has already been sent for this state
        if (!emailSentForSolidRed) 
        {
            // Send email
            Serial.println("Sending email for solid red state...");
            String emailBody = "The button has been in the solid red state.";
            notificationManager->sendEmail(
                "smtp.gmail.com", 465, 
                wifiManager->getSenderEmail().c_str(), 
                wifiManager->getSenderPassword().c_str(), 
                wifiManager->getEmailSubject().c_str(), 
                emailBody.c_str(),
                configRedLED
            );
            // Mark email as sent
            emailSentForSolidRed = true;
        }

        if (!messageSent)
        {
            notificationManager->sendSMS(
            "ACf421c9e76d3e2c2914b1138c3c03b214",
            wifiManager->getAuthToken(),
            "+18449893949",             // Sender must be in E.164 format, e.g., "+1234567890"
            wifiManager->getPhone(),   // Receiver must be in E.164 format, e.g., "+1987654321"
            wifiManager->getEmailBody()   
        );
        messageSent = true;
        }
    }
    else if (isTimeWithinRange(String(currentTime), targetTime, 1800)) { // 30 minutes 
        setButtonState("flashingRed");
    }
    else if (isTimeWithinRange(String(currentTime), targetTime, 3600)) { // 1 hour
        setButtonState("blue");
    }
}

void ButtonManager::printTimeLeftToAlarm() 
{
    // Get current time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    char currentTimeStr[6];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M", &timeinfo);

    // Parse current time
    int currentHour, currentMinute, targetHour, targetMinute;
    sscanf(currentTimeStr, "%d:%d", &currentHour, &currentMinute);
    sscanf(targetTime.c_str(), "%d:%d", &targetHour, &targetMinute);

    // Convert times to minutes
    int nowMinutes = currentHour * 60 + currentMinute;
    int targetMinutes = targetHour * 60 + targetMinute;

    int diff = targetMinutes - nowMinutes;

    if (diff < 0) {
        Serial.println("Alarm time has already passed for today.");
        return;
    }

    int hoursLeft = diff / 60;
    int minutesLeft = diff % 60;

    Serial.printf("Time left until alarm: %d hour(s) and %d minute(s)\n", hoursLeft, minutesLeft);
}

void ButtonManager::setLED(int pin, bool state, bool activeLow) {
    if (activeLow) {
        digitalWrite(pin, state ? LOW : HIGH); // Active LOW
    } else {
        digitalWrite(pin, state ? HIGH : LOW); // Active HIGH
    }
}

void ButtonManager::flashRedLED() {
    // Implement flashing red logic for WS2812B LEDs
    static unsigned long lastFlashTime = 0;
    static bool ledState = false;
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= 500) { // Toggle every 500ms
        ledState = !ledState;
        if (ledState) {
            setMainLEDsRed();
        } else {
            setMainLEDsOff();
        }
        lastFlashTime = currentTime;
    }
}

void ButtonManager::flashConfigBlueLED() {
    // Implement flashing blue logic for configuration mode
    static unsigned long lastFlashTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= 500) { // Toggle every 500ms
        digitalWrite(configBlueLED, !digitalRead(configBlueLED));
        lastFlashTime = currentTime;
    }
}

void ButtonManager::flashConfigRedLED() {
    // Implement flashing red logic when WiFi connection fails
    static unsigned long lastFlashTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= 500) { // Toggle every 500ms
        digitalWrite(configRedLED, !digitalRead(configRedLED));
        lastFlashTime = currentTime;
    }
}

void ButtonManager::blinkConfigGreenLED() {
    // Blink the green LED five times rapidly
    for (int i = 0; i < 10; i++) { // Blink 5 times (10 toggles)
        setLED(configGreenLED, true, true); // Turn on the green LED (active LOW)
        delay(100); // Wait for 100ms
        setLED(configGreenLED, false, true); // Turn off the green LED (active LOW)
        delay(100); // Wait for 100ms
    }
}

void ButtonManager::startConnectivityMode() {
    apMode = true;
    wifiManager->eraseConfig();         // Erase previous configuration
    wifiManager->begin();
    setLED(configBlueLED, true, true); // Start flashing blue LED (active LOW)
    setLED(configRedLED, false, true); // Turn off the red LED (active LOW)
    wifiConnected = false; // Reset WiFi connection status
    Serial.println("Connectivity mode started. Access point is up.");
}

void ButtonManager::startVacationMode() 
{
    setMainLEDsBlue(); // Turn on the blue LEDs to indicate vacation mode
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
            setLED(configBlueLED, !digitalRead(configBlueLED), true); // Toggle blue LED rapidly
            delay(100);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi connected.");
            wifiConnected = true;
            blinkConfigGreenLED(); // Blink the green LED five times rapidly to indicate successful connection
            setLED(configBlueLED, false, true); // Ensure the blue LED is turned off
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
    setLED(configRedLED, true, true); // Turn on the red LED (active LOW)
    wifiConnected = false;

    // Automatically enter configuration mode after failure
    startConnectivityMode();
}

void ButtonManager::indicateConfigurationNeeded() {
    // Indicate that configuration is needed with a solid red LED
    setLED(configRedLED, true, true); // Turn on the red LED (active LOW)
    Serial.println("Configuration is needed.");
}

int ButtonManager::getConsecutivePressCount() {
    return consecutivePressCount;
}

void ButtonManager::resetConsecutivePressCount() {
    consecutivePressCount = 0;
}

