


# CheckInCare Patient Device User Guide

CheckInCare is an ESP32-based patient monitoring device designed for home care environments. It provides automated check-in reminders, real-time notifications, and multi-modal alerts to ensure patient safety and caregiver awareness.

---

## Features

- **📱 Smart Check-in Monitoring:** Configurable reminder times with warning threshold alerts
- **🔊 Audio Feedback:** I2S audio amplifier with buzzer tones and SD card MP3 support
- **🌈 Visual Indicators:** WS2812B RGB LED strip for status indication
- **📧 Email Notifications:** Gmail SMTP integration for instant alerts
- **📱 SMS Notifications:** Twilio integration for text message alerts
- **🏝️ Vacation Mode:** Temporary monitoring suspension for planned absences
- **⚙️ Web Configuration:** User-friendly browser-based setup interface
- **🌐 NTP Time Sync:** Automatic time synchronization with timezone support
- **💾 SD Card Support:** Custom audio files for personalized reminder sounds

---

## Hardware Components

- **ESP32-S3 DevKit:** Main microcontroller with WiFi capability
- **MAX98357A I2S Audio Amplifier:** High-quality audio output (Pins 40-42)
- **WS2812B LED Strip:** 4-LED addressable RGB strip (Pin 38) 
- **SIM800C GSM Module:** Optional SMS support (Pins 17-18)
- **SD Card Module:** Custom audio file storage
- **Push Button:** Main user interface (Pin 48)
- **Configuration LEDs:** Status indicators (Pins 4-6)

---

## Getting Started

### Initial Setup
1. **Power on the device** and wait for the startup sequence to complete
2. **Press and hold the button for 10+ seconds** to enter configuration mode
3. The device will create a **WiFi Access Point (AP)**:
   - **SSID:** `ESP32-Access-Point`
   - **Password:** `123456789`
4. Connect your smartphone or computer to this AP
5. Wait for the configuration portal to appear automatically, or navigate to `192.168.4.1`

### Device Configuration
The web interface provides comprehensive configuration options:

#### WiFi Settings
- **WiFi SSID:** Your home WiFi network name
- **WiFi Password:** Your WiFi network password

#### Notification Settings  
- **Recipient Email:** Email address to receive alerts
- **Phone Number:** Mobile number for SMS alerts (format: +1234567890)
- **Sender Email:** Gmail account for sending notifications
- **Sender Password:** Gmail app password (not regular password)
- **Auth Token:** Twilio authentication token for SMS
- **Email Subject:** Custom subject line for notifications
- **Message Body:** Default message content for alerts

#### Monitoring Settings
- **Button Press Time:** Daily check-in time (HH:MM format)
- **Warning Threshold:** Minutes before check-in time to start alerts (1-60)
- **Time Zone:** Local timezone (UTC-12 to UTC+14)

### Advanced Audio Setup (Optional)
To use custom reminder sounds:
1. Format an SD card as FAT32
2. Place an MP3 file named `reminder.mp3` in the root directory
3. Insert the SD card into the device
4. The device will automatically use your custom sound, falling back to generated tones if the file is unavailable

---

## Button Functions

The device features a sophisticated multi-function button interface:

### Single Press (< 3 seconds)
- **Action:** Manual check-in / LED activation
- **LED Response:** Changes to white
- **Effect:** Resets daily monitoring, skips alarm for current day
- **Notifications:** Clears all daily alert flags

### Press and Hold (3-10 seconds)  
- **Action:** Enter vacation mode
- **LED Response:** Blue indication for 3 seconds, then steady blue
- **Effect:** Suspends all monitoring until manually exited
- **Exit:** Short press (< 3 seconds) to resume normal operation

### Press and Hold (10+ seconds)
- **Action:** Enter configuration mode
- **LED Response:** Flashing blue during setup
- **Effect:** Creates WiFi AP for device reconfiguration
- **Auto-exit:** Completes when configuration is saved or existing WiFi reconnects

### Four Rapid Presses (< 0.4 seconds each)
- **Action:** Emergency alert
- **Notifications:** Sends immediate email and SMS to caregivers
- **Message:** Uses configured custom message or default emergency text

---

## LED Status Indicators

### Main LED Strip (WS2812B - 4 LEDs)
- **🤍 White (Solid):** Normal operation / Check-in completed
- **🔵 Blue (Solid):** Vacation mode active / 1 hour before check-in time  
- **🔴 Red (Slow Flash):** Warning period active (30 minutes to warning threshold)
- **🔴 Red (Rapid Flash):** Urgent warning (within warning threshold minutes)
- **🔴 Red (Solid):** Check-in time reached / Missed check-in
- **⚫ Off:** Device idle / Deep sleep mode

### Configuration LEDs (Status Indicators)
- **🔵 Blue (Flashing):** Configuration mode active / WiFi setup in progress
- **🔴 Red (Flashing):** WiFi connection failed / Network connectivity issues  
- **🟢 Green (5x Rapid Blink):** Successful WiFi connection / Configuration saved

---

## Audio System

The device includes a sophisticated audio feedback system using the MAX98357A I2S amplifier:

### Reminder Sounds
- **Standard:** 3 gentle beeps at 800Hz
- **Custom MP3:** Plays `/reminder.mp3` from SD card if available
- **Timing:** Triggered at warning threshold and urgent warning phases

### Emergency Sounds  
- **Urgent Alert:** 5 rapid beeps at 1200Hz for immediate attention
- **Target Time:** 2 long beeps at 1000Hz when check-in time arrives

### Audio File Support
- **Format:** MP3 files via SD card
- **Location:** Root directory of FAT32-formatted SD card
- **Filename:** Must be named `reminder.mp3`
- **Fallback:** Automatically uses generated tones if MP3 unavailable

---

## Notification System

### Email Notifications (Gmail SMTP)
- **Check-in Alerts:** Sent when target time is reached
- **Missed Check-in:** Sent 5 minutes after target time if button not pressed
- **Emergency Alerts:** Sent immediately on 4-press sequence
- **Requirements:** Gmail account with app password enabled

### SMS Notifications (Twilio)
- **Dual Delivery:** All email alerts also sent via SMS
- **International Support:** Works worldwide with proper phone number formatting
- **Requirements:** Twilio account with authentication token

### SIM800C GSM Module (Optional)
- **Direct SMS:** Send SMS without internet connection
- **APN Configuration:** Supports custom carrier settings
- **Fallback:** Alternative to Twilio for areas with poor WiFi

---

## Daily Monitoring Workflow

### Normal Day Sequence
1. **Device monitors time** continuously after WiFi connection
2. **1 hour before:** Blue LEDs indicate approaching check-in time
3. **30 minutes before:** Slow red flashing begins
4. **Warning threshold:** Rapid red flashing + audio reminder
5. **Target time:** Solid red + caregiver notifications
6. **Button press:** Resets to white, monitoring complete for day

### Monitoring States
- **Default:** White LEDs, quiet monitoring
- **Approaching:** Blue LEDs, 1-hour warning  
- **Warning:** Slow red flash, 30-minute window
- **Urgent:** Rapid red flash + audio within threshold
- **Alert:** Solid red + notifications at target time
- **Missed:** Continued red + escalated notifications after 5 minutes

### Automatic Reset
- Daily monitoring resets at target time
- Button press any time resets current day
- Vacation mode suspends all monitoring
- Device startup includes 2-minute grace period

---

## Time Zone and NTP Synchronization

The device automatically synchronizes time using Network Time Protocol (NTP):

### Supported Time Zones
- **Range:** UTC-12 to UTC+14 (covers all global time zones)
- **Special Zones:** Includes half-hour and 45-minute offsets
- **Examples:** UTC-8 (PST), UTC+5:30 (IST), UTC+9:30 (ACST)

### Time Configuration
- **NTP Server:** pool.ntp.org (automatic failover)
- **Sync Frequency:** Automatic resync on WiFi reconnection
- **Display Format:** 24-hour format (HH:MM)
- **Date Format:** ISO 8601 (YYYY-MM-DD)

---

## Vacation Mode

Vacation mode provides a way to temporarily suspend monitoring:

### Activation
1. Press and hold button for 3-10 seconds
2. Blue LEDs will illuminate for 3 seconds as confirmation
3. LEDs remain steady blue during vacation mode
4. All monitoring and notifications are suspended

### During Vacation Mode
- **LED Status:** Steady blue indication
- **Monitoring:** Completely disabled
- **Notifications:** No emails or SMS sent
- **Time Tracking:** Target time ignored
- **Power Usage:** Reduced CPU frequency for battery conservation

### Exiting Vacation Mode
1. **Short press** button (< 3 seconds) at any time
2. LEDs change to white immediately
3. Normal monitoring resumes
4. Daily flags reset for fresh start

---

## Power Management

The device includes intelligent power management features:

### Dynamic CPU Scaling
- **Active Mode:** 240 MHz during alerts and configuration
- **Idle Mode:** 80 MHz during normal monitoring (power saving)
- **Vacation Mode:** Reduced frequency for extended battery life

### Sleep Features
- **Light Sleep:** Implemented during idle periods
- **Wake Triggers:** Button press, timer events, network activity
- **Deep Sleep:** Available for extended power savings (optional)

---

## Troubleshooting

### WiFi Connection Issues
**Symptoms:** Red LED flashing, no notifications sent
**Solutions:**
- Verify SSID and password are correct
- Check WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure network allows new device connections
- Try re-entering configuration mode (hold button 10+ seconds)
- Check for special characters in WiFi credentials

### Email Notification Failures  
**Symptoms:** No email alerts received, error messages in serial monitor
**Solutions:**
- Verify sender email is a Gmail account
- Use Gmail app password, not regular account password
- Enable 2-factor authentication on Gmail account
- Check spam/junk folder for alerts
- Verify recipient email address is correct
- Test with simple email subject/body first

### SMS Notification Issues
**Symptoms:** Emails work but no SMS received
**Solutions:**
- Verify Twilio account credentials are correct
- Check phone number format (+country code + number)
- Ensure sufficient Twilio account balance
- Verify "From" number is valid for your region
- Check carrier SMS blocking settings

### Audio System Problems
**Symptoms:** No sound output, distorted audio
**Solutions:**
- Check I2S wiring (pins 40, 41, 42)
- Verify MAX98357A power supply (3.3V or 5V)
- Test SD card MP3 files are valid format
- Check SD card FAT32 formatting
- Ensure speaker is connected to amplifier output
- Verify SD_MODE pin (39) enables amplifier

### Time Synchronization Issues  
**Symptoms:** Wrong time displayed, alerts at incorrect times
**Solutions:**
- Verify WiFi connection is stable
- Check selected time zone matches your location
- Allow several minutes for initial NTP sync
- Restart device to force fresh time sync
- Test with known working NTP servers

### Button Response Problems
**Symptoms:** Button presses not detected, multiple press issues  
**Solutions:**
- Check button wiring to pin 48
- Verify pullup resistor configuration
- Clean button contacts if sticky
- Check for electromagnetic interference
- Adjust debounce settings if needed
- Test with different button if available

### Configuration Web Page Issues
**Symptoms:** Cannot access 192.168.4.1, page doesn't load
**Solutions:**
- Ensure device is in configuration mode (blue LED flashing)
- Verify connection to ESP32-Access-Point
- Try different browser or device
- Clear browser cache and cookies
- Check if device created AP (ESP32-Access-Point in WiFi list)
- Wait for full device startup (2-3 minutes after power on)

---

## Technical Specifications

### Microcontroller
- **Model:** ESP32-S3 DevKit-C-1
- **CPU:** Dual-core Xtensa LX7, 240 MHz  
- **Memory:** 512KB SRAM, 384KB ROM
- **Flash:** 8MB (programmable)
- **WiFi:** 802.11 b/g/n, 2.4 GHz
- **Bluetooth:** 5.0 LE support

### Power Requirements
- **Operating Voltage:** 3.3V - 5V DC
- **Current Consumption:** 
  - Active: ~200mA
  - Idle: ~80mA  
  - Deep Sleep: <10mA
- **Power Supply:** USB-C connector or external 5V

### Environmental Specifications
- **Operating Temperature:** -10°C to +50°C
- **Humidity:** 20% to 80% non-condensing
- **Storage Temperature:** -20°C to +70°C

### Connectivity
- **WiFi Protocols:** WPA/WPA2/WPA3 security
- **Email Protocol:** SMTP over SSL/TLS (Port 465)
- **SMS Protocol:** Twilio REST API over HTTPS
- **Time Protocol:** NTP over UDP (Port 123)

---

## Maintenance

### Regular Maintenance
- **Weekly:** Check LED status and WiFi connectivity
- **Monthly:** Test notification system with 4-press sequence
- **Quarterly:** Update device configuration if needed
- **Annually:** Check battery backup (if installed)

### Software Updates
- Monitor for firmware updates
- Backup device configuration before updates
- Test all functions after updating

### Hardware Maintenance  
- Keep device clean and dust-free
- Check cable connections periodically
- Replace SD card if audio issues occur
- Verify button mechanical operation

---

## Support and Resources

### Getting Help
- **Serial Monitor:** Enable for detailed debug information
- **Error Codes:** Check console output for specific error messages
- **Documentation:** Refer to this guide for troubleshooting steps

### Advanced Configuration
- **PlatformIO:** Development environment for firmware modifications
- **Libraries Used:** 
  - ESP Mail Client v3.4.24 (Email)
  - Adafruit NeoPixel (LED control)
  - NTPClient (Time synchronization)
  - ESP32 SD library (File system)

### Device Information
- **Firmware Version:** v2.0
- **Hardware Revision:** CheckInCare v2
- **Compile Date:** Check serial output on startup
- **Memory Usage:** Monitor via serial console

---

Enjoy using your CheckInCare device for enhanced patient safety and peace of mind!