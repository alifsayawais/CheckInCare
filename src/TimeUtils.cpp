#include "TimeUtils.h"
#include <WiFi.h>
#include <time.h>

// NTP server and timezone configuration
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = 0;       // Default to UTC
int daylightOffset_sec = 0;   // No daylight savings by default

// Function to configure the time zone
void configureTimeZone(const String& timeZone) {
    // Default to UTC if time zone is invalid
    const String defaultTimeZone = "UTC+0";

    String effectiveTimeZone = timeZone;
    if (!timeZone.startsWith("UTC") || timeZone.length() < 4) {
        Serial.println("Invalid time zone format. Falling back to default: " + defaultTimeZone);
        effectiveTimeZone = defaultTimeZone;
    }

    // Extract the offset in hours
    String offsetStr = effectiveTimeZone.substring(3);
    int offsetHours = offsetStr.toInt();
    gmtOffset_sec = offsetHours * 3600; // Convert hours to seconds

    // Configure NTP client with the effective time zone
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

// Function to fetch the current time as a formatted string
String getCurrentTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return "N/A";
    }
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeStr);
}