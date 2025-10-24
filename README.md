


# CheckInCare Device User Guide

This guide provides step-by-step instructions for using the CheckInCare device.

---

## Getting Started

### Initial Setup
1. Power on the device and Press the button for 10 seconds.
2. The device will create an **Access Point (AP)**:
   - **SSID:** `ESP32-Access-Point`
   - **Password:** `12345678`
3. Connect your smartphone or computer to this AP.

### Configuring the Device
1. Open a web browser and go to `192.168.4.1`.
2. Fill out the displayed configuration form:
   - **WiFi SSID:** Enter your WiFi network name.
   - **WiFi Password:** Enter your WiFi password.
   - **Your Email:** Provide your email address for notifications.
   - **Phone (Optional):** Enter your phone number for SMS alerts.
   - **Sender Email:** Enter the email address to send notifications.
   - **Sender Password:** Enter the password for the sender email.
   - **Email Body (Optional):** Provide a default message for notifications.
3. Click "Submit" to save the configuration. The device will attempt to connect to your WiFi network.

---

## Using the Button

The device features a multi-functional button. Its actions are:

- **Single Press (< 3 seconds):** Activates the white LED.
- **Press and Hold (3-10 seconds):** Activates vacation mode (blue LED).
- **Press and Hold (> 10 seconds):** Enables connectivity mode for reconfiguration.
- **Four Rapid Presses (< 0.4 seconds each):** Sends a notification email.

---

## LED Indicators

- **White LED Main:** Normal operation.
- **Small Blue LED (Flashing):** Configuration mode is active.
- **Small Red LED (Solid):** Connectivity issue detected.
- **Small Green LED (Blinking):** Configuration successful.

---

## Notifications

- **Email Alerts:** Sent to the configured email address for specific button actions.
- **SMS Alerts (Optional):** If a SIM800 module is connected, SMS alerts can be sent to the configured phone number.

---

## Reconfiguring the Device

To erase the current configuration:
1. Press and hold the button for more than 10 seconds.
2. The red LED will flash to indicate the configuration has been erased.
3. Repeat the **Initial Setup** steps to reconfigure the device.

---

## Troubleshooting

- **WiFi Connection Issues:**
  - Double-check the entered SSID and password.
  - Reconfigure the device using connectivity mode (press and hold button for 10+ seconds).
- **Notification Issues:**
  - Verify the sender email and password are correct.
  - Ensure the SMTP server is reachable.

---

Enjoy using CheckInCare!
