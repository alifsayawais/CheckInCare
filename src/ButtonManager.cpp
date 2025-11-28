#include "ButtonManager.h"
#include "NotificationManager.h"
#include "TimeUtils.h"

ButtonManager::ButtonManager(int mainButton, int configBlueLED, int configGreenLED, int configRedLED, WiFiManager* wifiManager, NotificationManager* notificationManager)
    : mainButton(mainButton), configBlueLED(configBlueLED), configGreenLED(configGreenLED), configRedLED(configRedLED),
      lastPressTime(0), pressInterval(12 * 60 * 60 * 1000), debounceDelay(50), lastDebounceTime(0), lastButtonState(HIGH), buttonState(HIGH), apMode(false), wifiManager(wifiManager), connectivityModeStarted(false), wifiConnected(false), vacationModeStarted(false), validPressStarted(false), consecutivePressCount(0), lastPressCheckTime(0), emailSentForSolidRed(false), messageSent(false), twentyMinWarningShown(false), missedButtonEmailSent(false), deviceJustStarted(true), startupTime(millis()), connectivityModeStartTime(0) {
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
    // Handle rapid red LED flashing for 20-minute warning
    if (currentState == "rapidRed") {
        flashRapidRedLED();
    }
    
    // Power saving: If device is idle (no alerts, connected to WiFi), reduce CPU frequency
    static unsigned long lastPowerSaveCheck = 0;
    if (millis() - lastPowerSaveCheck >= 60000) { // Check every minute
        if (wifiConnected && !apMode && !vacationModeStarted && !connectivityModeStarted && 
            currentState != "flashingRed" && currentState != "rapidRed" && currentState != "solidRed") {
            
            // Device is idle - enable power saving
            Serial.println("💡 Device idle - enabling power saving mode");
            setCpuFrequencyMhz(80); // Reduce from 240MHz to 80MHz
            
            // Could also use light sleep for short periods
            // esp_sleep_enable_timer_wakeup(1000000); // 1 second
            // esp_light_sleep_start();
        } else {
            // Restore full performance when active
            setCpuFrequencyMhz(240);
        }
        lastPowerSaveCheck = millis();
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
                
                // Don't reset connectivity mode flag if already in connectivity mode
                if (!connectivityModeStarted) {
                    // Only reset these flags if not in connectivity mode
                    // vacationModeStarted = false; // Reset flag for vacation mode
                }
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
                    
                    // Send SMS via Twilio
                    String phoneNumber = wifiManager->getPhone();
                    if (!phoneNumber.isEmpty()) {
                        notificationManager->sendTwilioSMS(phoneNumber, emailBody);
                    }
                    
                    // Automatically release the button and reset the press count
                    buttonState = HIGH; // Simulate button release
                    consecutivePressCount = 0; // Reset the press count
                }
            } 
            else if (buttonState == HIGH && (millis() - lastPressTime < 3000) && !connectivityModeStarted) 
            {
                Serial.println("Button released before 3 seconds");
                Serial.println("Single press detected - turning LEDs white");
                validPressStarted = false; // Reset valid press flag on release
                resetTimer();
                Serial.print("Vacation mode flag before exit: ");
                Serial.println(vacationModeStarted);
                if (vacationModeStarted) {
                    vacationModeStarted = false;
                    Serial.println("Vacation mode exited due to short press.");
                }
                alarmSkipDate = getTodayDate();
                Serial.print("Alarm skipped for today: ");
                Serial.println(alarmSkipDate);
                
                // Reset daily notification flags when button is pressed
                twentyMinWarningShown = false;
                missedButtonEmailSent = false;
                emailSentForSolidRed = false;
                messageSent = false;
                
                // Always turn LEDs white on single press
                setMainLEDsWhite();
                currentState = "white";
            }
            // Handle button press during connectivity mode
            else if (buttonState == HIGH && (millis() - lastPressTime < 3000) && connectivityModeStarted) 
            {
                // Add grace period to prevent immediate exit after starting connectivity mode
                if (millis() - connectivityModeStartTime < 2000) { // 2-second grace period
                    Serial.println("Button press ignored - connectivity mode grace period active");
                    validPressStarted = false;
                    return;
                }
                
                Serial.println("Button pressed during connectivity mode");
                validPressStarted = false; // Reset valid press flag on release
                
                // Check if there's existing WiFi configuration
                if (!wifiManager->getSSID().isEmpty()) {
                    Serial.println("Existing configuration found - exiting connectivity mode");
                    connectivityModeStarted = false;
                    apMode = false;
                    
                    // Turn off config LEDs
                    setLED(configBlueLED, false, true);
                    setLED(configRedLED, false, true);
                    
                    // Try to connect with existing configuration
                    tryConnectWiFi();
                } else {
                    Serial.println("No existing configuration - staying in connectivity mode");
                    // Stay in connectivity mode for setup
                }
            }
            else if (buttonState == HIGH && (millis() - lastPressTime >= 3000) && (millis() - lastPressTime < 10000) && !vacationModeStarted && !connectivityModeStarted) { // Button released after 3-10 seconds
                Serial.println("Button released after 3-10 seconds");
                Serial.print("Hold duration: ");
                Serial.print((millis() - lastPressTime) / 1000);
                Serial.println(" seconds");
                Serial.println("Starting vacation mode - turning LEDs BLUE");
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
        connectivityModeStartTime = millis(); // Record when connectivity mode started
        vacationModeStarted = false; // Ensure vacation mode is not started
        validPressStarted = false; // Reset the valid press flag
        buttonPressed = true; // Set button pressed flag
        lastPressTime = millis(); // Reset press time for next button detection
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
    if (state == currentState) {
        return;
    }

    Serial.print("Button state changed to: ");
    Serial.println(state);
    if (state == "blue") 
    {
        setMainLEDsBlue();
    } 
    else if (state == "flashingRed") 
    {
        // Red flashing handled in update() method
        // Play reminder sound when starting warning phase
        if (currentState != "flashingRed") {
            playReminderSound(); // Play gentle reminder sound
        }
    } 
    else if (state == "rapidRed") 
    {
        // Rapid red flashing handled in update() method  
        // Play urgent sound when entering urgent warning phase
        if (currentState != "rapidRed") {
            //playUrgentSound(); // Commented out for now
        }
    }
    else if (state == "solidRed") 
    {
        setMainLEDsRed();
        // Play target time sound when check-in time is reached
        if (currentState != "solidRed") {
            //playTargetTimeSound(); // Commented out for now
        }
    }
    else if (state == "white")
    {
        setMainLEDsWhite();
    } 
    else if (state == "off")
    {
        setMainLEDsOff();
    }

    // Turn off flashing LED when leaving flashingRed or rapidRed (but not when going to solidRed)
    if ((currentState == "flashingRed" || currentState == "rapidRed") && 
        (state != "flashingRed" && state != "rapidRed" && state != "solidRed")) {
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
    // Handle vacation mode with simple timing (no sleep)
    if (vacationModeStarted) {
        // Check if button was pressed to exit vacation mode
        if (digitalRead(mainButton) == LOW) {
            Serial.println("🔘 Button pressed - checking for vacation mode exit...");
            delay(100); // Debounce
            
            // Wait for button release and measure hold time
            unsigned long pressStart = millis();
            while (digitalRead(mainButton) == LOW) {
                delay(10);
                // If held too long, ignore (prevent accidental exit during long press)
                if (millis() - pressStart > 3000) {
                    Serial.println("Long press detected in vacation mode - ignoring");
                    return;
                }
            }
            
            unsigned long pressDuration = millis() - pressStart;
            if (pressDuration < 3000) { // Short press = exit vacation mode
                Serial.println("✅ Short press detected - exiting vacation mode!");
                vacationModeStarted = false;
                setMainLEDsWhite();
                currentState = "white";
                alarmSkipDate = getTodayDate();
                Serial.println("Vacation mode exited - normal operation resumed");
                
                // Reset all daily flags
                twentyMinWarningShown = false;
                missedButtonEmailSent = false;
                emailSentForSolidRed = false;
                messageSent = false;
                
                return; // Exit vacation mode immediately
            }
        }
        
        // If still in vacation mode, just return without continuous LED updates
        return; // No other monitoring during vacation
    }
    
    // Skip all check-in logic when in connectivity mode
    if (connectivityModeStarted) {
        return; // No main LED changes during configuration
    }
    
    if (targetTime == "") {
        Serial.println("No target time set - monitoring disabled");
        return; // No target time set
    }
    
    // Debug output - only print once per minute
    static unsigned long lastDebugTime = 0;
    static String lastTimeDebug = "";
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        String currentTimeStr = String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min);
        if (millis() - lastDebugTime > 60000 || lastTimeDebug != currentTimeStr) { // Every 60 seconds or when minute changes
            Serial.print("Monitoring active - Current time: ");
            Serial.printf("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
            Serial.print(", Target time: ");
            Serial.println(targetTime);
            lastDebugTime = millis();
            lastTimeDebug = currentTimeStr;
        }
    }
    
    today = getTodayDate();
    
    if (alarmSkipDate == today)
    {
        if (currentState != "white") {
            setButtonState("white");
        }
        // Reset daily flags when alarm is skipped
        twentyMinWarningShown = false;
        missedButtonEmailSent = false;
        emailSentForSolidRed = false;
        messageSent = false;
        return;
    }
    
    // Get the current time (hour and minute)
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    char currentTime[6];
    strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);

    // Check if target time has passed (send missed button email only once)
    if (!missedButtonEmailSent) {
        // Check startup protection only once and disable it after 2 minutes
        if (deviceJustStarted) {
            if ((millis() - startupTime) < 120000) { // 2 minutes = 120000ms
                // Still in startup grace period - skip email logic silently
                return;
            } else {
                // Grace period ended - disable startup protection
                deviceJustStarted = false;
                Serial.println("Device startup grace period ended - resuming normal email monitoring");
            }
        }
        
        // Parse times for comparison
        int currentHour, currentMinute, targetHour, targetMinute;
        sscanf(currentTime, "%d:%d", &currentHour, &currentMinute);
        sscanf(targetTime.c_str(), "%d:%d", &targetHour, &targetMinute);
        
        time_t now = currentHour * 3600 + currentMinute * 60;
        time_t target = targetHour * 3600 + targetMinute * 60;
        
        // Check if current time is more than 5 minutes past target time
        if (now > target && (now - target) >= 300) { // 5 minutes = 300 seconds
            Serial.println("Target time passed by more than 5 minutes - sending missed button alert");
            Serial.print("Current time: ");
            Serial.print(currentHour);
            Serial.print(":");
            Serial.println(currentMinute);
            Serial.print("Target time: ");
            Serial.print(targetHour);
            Serial.print(":");
            Serial.println(targetMinute);
            Serial.print("Time difference: ");
            Serial.print((now - target) / 60);
            Serial.println(" minutes");
            
            String emailBody = "ALERT: The patient has not pressed the button at the scheduled time (";
            emailBody += targetTime;
            emailBody += "). Please check on them immediately.";
            
            // Send email notification
            notificationManager->sendEmail(
                "smtp.gmail.com", 465, 
                wifiManager->getSenderEmail().c_str(), 
                wifiManager->getSenderPassword().c_str(), 
                "URGENT: Missed Check-in Alert", 
                emailBody.c_str(),
                configRedLED
            );
            
            // Send SMS notification via Twilio
            String phoneNumber = wifiManager->getPhone();
            if (!phoneNumber.isEmpty()) {
                Serial.print("Sending SMS alert to: ");
                Serial.println(phoneNumber);
                notificationManager->sendTwilioSMS(phoneNumber, emailBody);
            } else {
                Serial.println("No phone number configured - skipping SMS alert");
            }
            
            missedButtonEmailSent = true;
            setButtonState("solidRed"); // Keep solid red to indicate missed check-in
            return;
        }
    }

    // Configurable warning threshold (cached to avoid repeated parsing)
    static int cachedWarningMinutes = -1;
    static String cachedThresholdStr = "";
    
    String thresholdStr = wifiManager->getWarningThreshold();
    int warningMinutes;
    
    if (thresholdStr != cachedThresholdStr) {
        // Only parse and debug when threshold changes
        Serial.print("Raw threshold from server: '");
        Serial.print(thresholdStr);
        Serial.println("'");
        warningMinutes = thresholdStr.toInt();
        if (warningMinutes <= 0) {
            warningMinutes = 20; // Default to 20 minutes if invalid
            Serial.println("Using default 20 minutes (server value was invalid)");
        } else {
            Serial.print("Using server configured value: ");
            Serial.print(warningMinutes);
            Serial.println(" minutes");
        }
        cachedWarningMinutes = warningMinutes;
        cachedThresholdStr = thresholdStr;
    } else {
        // Use cached value
        warningMinutes = cachedWarningMinutes;
    }
    
    int warningSeconds = warningMinutes * 60;
    
    if (!twentyMinWarningShown && isTimeWithinRange(String(currentTime), targetTime, warningSeconds)) {
        Serial.print("Warning threshold reached (");
        Serial.print(warningMinutes);
        Serial.println(" minutes) - rapid red flashing");
        setButtonState("rapidRed");
        playReminderSound(); // Play reminder sound when warning threshold is reached
        twentyMinWarningShown = true;
        return;
    }
    
    if (String(currentTime) == targetTime || isTimeWithinRange(String(currentTime), targetTime, 30)) 
    { // Exact target time (with 30-second window) - just change LED state, no email to caregiver yet
        // Parse times to check if we're at or past target time
        int currentHour, currentMinute, targetHour, targetMinute;
        sscanf(currentTime, "%d:%d", &currentHour, &currentMinute);
        sscanf(targetTime.c_str(), "%d:%d", &targetHour, &targetMinute);
        
        time_t now = currentHour * 3600 + currentMinute * 60;
        time_t target = targetHour * 3600 + targetMinute * 60;
        
        // Only set solid red if we're at or past the target time
        if (now >= target && !emailSentForSolidRed) {
            setButtonState("solidRed");
            Serial.println("Target time reached - sending immediate alert to caregiver");
            String alertBody = "ALERT: The patient has reached the scheduled check-in time (";
            alertBody += targetTime;
            alertBody += "). Please ensure they check in now.";
            // Send email notification
            notificationManager->sendEmail(
                "smtp.gmail.com", 465,
                wifiManager->getSenderEmail().c_str(),
                wifiManager->getSenderPassword().c_str(),
                "URGENT: Scheduled Check-in Alert",
                alertBody.c_str(),
                configRedLED
            );
            // Send SMS notification via Twilio
            String phoneNumber = wifiManager->getPhone();
            if (!phoneNumber.isEmpty()) {
                Serial.print("Sending SMS alert to: ");
                Serial.println(phoneNumber);
                notificationManager->sendTwilioSMS(phoneNumber, alertBody);
            } else {
                Serial.println("No phone number configured - skipping SMS alert");
            }
            emailSentForSolidRed = true; // Mark as processed for this time slot
            return;
        }
    }
    else if (isTimeWithinRange(String(currentTime), targetTime, 1800)) { // 30 minutes 
        // Get the configurable warning threshold
        String thresholdStr = wifiManager->getWarningThreshold();
        int warningMinutes = thresholdStr.toInt();
        if (warningMinutes <= 0) warningMinutes = 20; // Default to 20 minutes if invalid
        int warningSeconds = warningMinutes * 60;
        
        // Check if we're within the warning threshold
        if (isTimeWithinRange(String(currentTime), targetTime, warningSeconds)) {
            setButtonState("rapidRed"); // Rapid flashing within warning threshold
        } else {
            setButtonState("flashingRed"); // Normal flashing outside warning threshold
        }
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

void ButtonManager::flashRapidRedLED() {
    // Implement rapid flashing red logic for 20-minute warning
    static unsigned long lastFlashTime = 0;
    static bool ledState = false;
    unsigned long currentTime = millis();
    if (currentTime - lastFlashTime >= 200) { // Toggle every 200ms for rapid flashing
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
    setMainLEDsOff();                   // Turn off main LEDs during configuration
    setLED(configBlueLED, true, true); // Start flashing blue LED (active LOW)
    setLED(configRedLED, false, true); // Turn off the red LED (active LOW)
    wifiConnected = false; // Reset WiFi connection status
    Serial.println("Connectivity mode started. Access point is up.");
}

void ButtonManager::startVacationMode() 
{
    Serial.println("🏖️ Vacation mode started!");
    
    // Show blue LEDs as vacation mode indication for 3 seconds
    setMainLEDsBlue(); 
    Serial.println("💙 Showing vacation mode indication (3 seconds)...");
    delay(3000); // Show blue for 3 seconds
    
    // Keep blue LEDs on during vacation mode as indication
    setMainLEDsBlue();
    vacationModeStarted = true;
    Serial.println("Vacation mode active - press button to exit");
}

/*
void ButtonManager::configureSleepWakeup() 
{
    // DISABLED - Sleep functionality removed
    Serial.println("Sleep functionality disabled");
}
*/

/*
void ButtonManager::enterDeepSleep(unsigned long sleepTimeSeconds) 
{
    // DISABLED - Sleep functionality removed
    Serial.println("Deep sleep functionality disabled");
}
*/

void ButtonManager::tryConnectWiFi() {
    String ssid = wifiManager->getSSID();
    String password = wifiManager->getPassword();
    const int maxAttempts = 2;
    int attempt = 0;

    Serial.println("Connecting to WiFi...");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);

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
            
            // Force reload configuration to get latest settings
            wifiManager->reloadConfigFromNVS();
            
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

