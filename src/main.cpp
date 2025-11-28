#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "WifiManager.h"
#include "ButtonManager.h"
#include "NotificationManager.h"
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SD.h>
#include <FS.h>
#include <TimeUtils.h>
#include <math.h>
#include <driver/i2s.h>


const int userGreenLED = 7;
const int mainButton = 48;
const int mainLED = 38; // Restored to 38 since I2S_DOUT moved to 40
const int configRedLED = 4;
const int configGreenLED = 5;
const int configBlueLED = 6;

// MAX98357A I2S Audio Amplifier pins
const int I2S_DOUT = 40;  // Data out pin (DIN on MAX98357A)
const int I2S_BCLK = 41;  // Bit clock pin (BCLK on MAX98357A)  
const int I2S_LRC = 42;   // Left/Right clock pin (LRC on MAX98357A)
const int SD_MODE = 39;   // SD Mode pin for MAX98357A

// SIM800C GSM Module pins
const int SIM800_TX = 17;  // SIM800C RX pin
const int SIM800_RX = 18;  // SIM800C TX pin
const int SIM800_PWR = 9;  // SIM800C power control pin

#define I2S_PORT I2S_NUM_0

// WS2812B LED configuration using Adafruit NeoPixel
#define NUM_LEDS 4
#define LED_DATA_PIN mainLED

Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

// SIM800C Serial communication
HardwareSerial sim800(1); // Use UART1 for SIM800C


const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "123456789";

NotificationManager* notificationManager = nullptr;
WiFiManager wifiManager(ap_ssid, ap_password);
ButtonManager buttonManager(mainButton, configBlueLED, configGreenLED, configRedLED, &wifiManager, notificationManager);

// WS2812B LED control functions using NeoPixel
void setMainLEDs(uint32_t color) {
    for(int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void setMainLEDsRed() {
    Serial.println("Setting main LEDs to RED");
    setMainLEDs(strip.Color(255, 0, 0));
}

void setMainLEDsBlue() {
    Serial.println("Setting main LEDs to BLUE");
    setMainLEDs(strip.Color(0, 0, 255));
}

void setMainLEDsWhite() {
    Serial.println("Setting main LEDs to WHITE");
    setMainLEDs(strip.Color(255, 255, 255));
}

void setMainLEDsOff() {
    Serial.println("Setting main LEDs to OFF");
    setMainLEDs(strip.Color(0, 0, 0));
}

// Forward declarations
void playWavFromSD(const char* filename);

// I2S Audio functions for MAX98357A
void setupI2S() {
    // Initialize SD_MODE pin for MAX98357A
    pinMode(SD_MODE, OUTPUT);
    digitalWrite(SD_MODE, HIGH); // Enable MAX98357A (HIGH = enabled, LOW = shutdown)
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_set_clk(I2S_PORT, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    
    Serial.println("I2S Audio initialized for MAX98357A");
    Serial.println("SD_MODE pin set HIGH - amplifier enabled");
}

void generateTone(int frequency, int duration_ms) {
    Serial.print("Generating I2S tone: ");
    Serial.print(frequency);
    Serial.print("Hz for ");
    Serial.print(duration_ms);
    Serial.println("ms");
    
    const int sample_rate = 44100;
    const int samples = (sample_rate * duration_ms) / 1000;
    const int amplitude = 16000; // Increased volume for better audibility
    
    int16_t *samples_data = (int16_t*)malloc(samples * 4); // Stereo 16-bit
    
    if (samples_data == NULL) {
        Serial.println("ERROR: Failed to allocate memory for audio samples!");
        return;
    }
    
    for (int i = 0; i < samples; i++) {
        // Generate square wave
        int16_t sample = (i % (sample_rate / frequency) < (sample_rate / frequency / 2)) ? amplitude : -amplitude;
        
        // Stereo output (left and right channels)
        samples_data[i * 2] = sample;     // Left channel
        samples_data[i * 2 + 1] = sample; // Right channel
    }
    
    size_t bytes_written;
    esp_err_t result = i2s_write(I2S_PORT, samples_data, samples * 4, &bytes_written, portMAX_DELAY);
    
    if (result == ESP_OK) {
        Serial.print("I2S write successful: ");
        Serial.print(bytes_written);
        Serial.println(" bytes written");
    } else {
        Serial.print("I2S write failed with error: ");
        Serial.println(result);
    }
    
    free(samples_data);
    Serial.println("I2S tone generation complete");
}

void buzzerTone(int frequency, int duration) {
    generateTone(frequency, duration);
}

void playReminderSound() {
    Serial.println("Playing reminder sound");
    
    // First try to play reminder.mp3 from SD card
    if (SD.begin()) {
        if (SD.exists("/reminder.mp3")) {
            Serial.println("Found reminder.mp3 on SD card - playing MP3 file");
            playWavFromSD("/reminder.mp3");
            return;
        } else {
            Serial.println("reminder.mp3 not found on SD card");
        }
    } else {
        Serial.println("SD card not available");
    }
    
    // Fallback to generated tones if MP3 file not available
    Serial.println("Using generated tones for reminder");
    
    // Fallback to generated tone (3 beeps)
    for (int i = 0; i < 3; i++) {
        Serial.print("Beep ");
        Serial.println(i + 1);
        buzzerTone(800, 300); // 800Hz for 300ms
        delay(200); // Pause between beeps
    }
    Serial.println("Reminder sound complete");
}

void playUrgentSound() {
    Serial.println("Playing urgent reminder sound");
    // Play urgent tone (5 rapid beeps)
    for (int i = 0; i < 5; i++) {
        Serial.print("Urgent beep ");
        Serial.println(i + 1);
        buzzerTone(1200, 200); // 1200Hz for 200ms
        delay(100); // Short pause between beeps
    }
    Serial.println("Urgent sound complete");
}

void playTargetTimeSound() {
    Serial.println("Playing target time sound");
    // Play target time tone (2 long beeps)
    for (int i = 0; i < 2; i++) {
        Serial.print("Target time beep ");
        Serial.println(i + 1);
        buzzerTone(1000, 500); // 1000Hz for 500ms
        delay(300); // Pause between beeps
    }
    Serial.println("Target time sound complete");
}

// Play a WAV file from SD card via I2S
void playWavFromSD(const char* filename) {
    Serial.print("Attempting to play WAV file from SD: ");
    Serial.println(filename);
    if (!SD.begin()) {
        Serial.println("SD card initialization failed!");
        return;
    }
    File file = SD.open(filename);
    if (!file) {
        Serial.println("Failed to open WAV file!");
        return;
    }

    // Skip WAV header (typically 44 bytes)
    file.seek(44);

    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    size_t bytesRead;
    size_t bytesWritten;

    Serial.println("Streaming audio to I2S...");
    while ((bytesRead = file.read(buffer, bufferSize)) > 0) {
        i2s_write(I2S_PORT, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
    }
    file.close();
    Serial.println("WAV playback finished.");
}

void setup() 
{
    Serial.begin(115200);
    Serial.println("� Device starting up normally");
    
    // Initialize SIM800C
    pinMode(SIM800_PWR, OUTPUT);
    digitalWrite(SIM800_PWR, HIGH); // Power on SIM800C
    delay(2000); // Wait for SIM800C to boot
    sim800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);
    
    // Initialize I2S for MAX98357A audio amplifier
    setupI2S();
    
    // Initialize WS2812B LEDs on GPIO 38
    strip.begin();
    strip.setBrightness(255); // Set brightness to maximum (0-255)
    setMainLEDsOff(); // Start with LEDs off

    if (!EEPROM.begin(512)) {
        Serial.println("Failed to initialize EEPROM. Formatting NVS...");
        if (nvs_flash_erase() == ESP_OK) {
            Serial.println("NVS formatted successfully. Reinitializing EEPROM...");
            if (!EEPROM.begin(512)) {
                Serial.println("Failed to reinitialize EEPROM after formatting NVS.");
                return;
            }
        } else {
            Serial.println("Failed to format NVS.");
            return;
        }
    }
    buttonManager.begin();

    String email = wifiManager.getEmail();
    String phone = wifiManager.getPhone();
    notificationManager = new NotificationManager(email, phone);
    notificationManager->setSIM800Serial(&sim800); // Set SIM800 serial reference
    
    // Configure APN settings
    notificationManager->setAPN("jazzconnect.mobilinkworld.com"); // Default APN
    
    buttonManager.setNotificationManager(notificationManager);
    
    // Set the time zone first
    String timeZone = wifiManager.getTimeZone();
    configureTimeZone(timeZone);

    // Log the current time
    Serial.print("Current Time: ");
    Serial.println(getCurrentTime());

    String userTime = wifiManager.getButtonTime(); // Replace with actual user input
    buttonManager.setTargetTime(userTime);
    Serial.print("Target Time Set: ");
    Serial.println(userTime);
}

void loop() 
{
    buttonManager.handleClient();
    buttonManager.update();
}
