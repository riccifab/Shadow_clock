#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RTClib.h>
#include <Adafruit_ADS1X15.h>
#include <FastLED.h>
#include "DisplayHandler.h"
#include "SensorHandler.h"
#include "TimeSyncHandler.h"

// Replace with your WiFi credentials
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// LED configuration
const int NUM_LEDS = 60;
#define LED_PIN 2
CRGB leds[NUM_LEDS]; // Defined here so that it's accessible by DisplayHandler.

RTC_DS3231 rtc;
Adafruit_ADS1115 ads;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // NTP update interval: 60 sec

unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 60000;  // 60 seconds

const float THRESHOLD_DISTANCE = 20.0;  // threshold distance in cm

void setup() {
  Serial.begin(115200);
  
  // Initialize LED strip.
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  // Begin WiFi (connection will be managed in the loop).
  WiFi.begin(ssid, password);
  Serial.println("Initiating WiFi connection...");

  // Initialize RTC.
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Initialize ADS1115.
  ads.begin();

  lastWifiCheck = millis();
}

void loop() {
  // Update WiFi/NTP time if needed.
  updateTimeIfNeeded(lastWifiCheck, WIFI_CHECK_INTERVAL, rtc, timeClient, ssid, password);

  // Read current time from RTC.
  DateTime now = rtc.now();

  // Read sensors (using channels 0 and 1).
  int16_t sensorValue1 = ads.readADC_SingleEnded(0);
  int16_t sensorValue2 = ads.readADC_SingleEnded(1);

  // Convert sensor readings to distances.
  float distance1 = calibrateSensor(sensorValue1);
  float distance2 = calibrateSensor(sensorValue2);

  // Handle the fade-in effect when no presence is detected.
  static bool fading = false;
  static unsigned long fadeStartTime = 0;
  
  bool presenceDetected = (distance1 < THRESHOLD_DISTANCE || distance2 < THRESHOLD_DISTANCE);

  if (presenceDetected) {
    // If presence is detected, cancel any fading and show the clock.
    fading = false;
    DisplayHandler::showClock(now);
  } else {
    // Start fade–in if not already running.
    if (!fading) {
      fading = true;
      fadeStartTime = millis();
    }
    unsigned long elapsed = millis() - fadeStartTime;
    int brightness = map(elapsed, 0, 2000, 0, 255);
    if (brightness > 255) {
      brightness = 255;
    }
    DisplayHandler::showFullFade(brightness);
  }
  
  delay(100);  // Adjust loop delay as needed.
}
