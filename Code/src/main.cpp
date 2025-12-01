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
const char* ssid     = "Fabs";
const char* password = "Jiminy_AP";

// LED configuration
const int NUM_LEDS = 60;
#define LED_PIN D6
CRGB leds[NUM_LEDS]; // Defined so it's accessible by DisplayHandler.

RTC_DS3231 rtc;
Adafruit_ADS1115 ads;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // NTP update interval: 60 sec

unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 100000;  // 100 sec (adjust if needed)
// THRESHOLD_DISTANCE is currently set to -4000; adjust this value to match your sensor's output range.
const float THRESHOLD_DISTANCE = -3000;
enum class StandbyOverride { None, Red, Purple };
StandbyOverride standbyOverride = StandbyOverride::None;


void setup() {
  Serial.begin(115200);
  Serial.println("=== System Initialization ===");
  
  // Initialize LED strip.
  Serial.println("Initializing LED strip...");
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  DisplayHandler::showFullFade(255);
  Serial.println("LED strip initialized.");

  // Begin WiFi (connection will be managed in the loop).
  Serial.println("Starting WiFi connection...");
  WiFi.begin(ssid, password);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);

  // Initialize RTC and ADS1115.
  Serial.println("Initializing RTC and ADS1115...");
  int brightness = 0;
  bool increasing = true;
  while (!rtc.begin() || !ads.begin(0x49)) {
    DisplayHandler::showFullFade(brightness);
    Serial.println("Couldn't find RTC or ADS1115! Retrying...");
    delay(50); // Pause briefly
    
    // Fade effect for visual feedback during initialization.
    if (increasing) {
      brightness += 5;
      if (brightness >= 255) {
        brightness = 255;
        increasing = false;
      }
    } else {
      brightness -= 5;
      if (brightness <= 0) {
        brightness = 0;
        increasing = true;
      }
    }
  }
  Serial.println("RTC and ADS1115 initialized successfully.");
  DisplayHandler::showFullFade(255);  
  lastWifiCheck = millis();
  Serial.println("Setup complete.");
}

void loop() {
  static unsigned long lastLoopTime = millis();
  unsigned long currentTime = millis();
  Serial.print("\n--- New Loop Iteration (");
  Serial.print(currentTime);
  Serial.println(" ms) ---");
  
  Serial.print("Time since last loop: ");
  Serial.print(currentTime - lastLoopTime);
  Serial.println(" ms");
  lastLoopTime = currentTime;

  // ───── serial commands ─────
  while (Serial.available() > 0) {
    char c = Serial.read();
    switch (c) {
      case 'r': case 'R':
        standbyOverride = StandbyOverride::Red;
        Serial.println(F("Standby colour → RED"));
        break;
      case 'p': case 'P':
        standbyOverride = StandbyOverride::Purple;
        Serial.println(F("Standby colour → PURPLE"));
        break;
      case 'n': case 'N':
        standbyOverride = StandbyOverride::None;
        Serial.println(F("Standby colour → NORMAL FADE"));
        break;
    }
  }

  // Update WiFi/NTP time if needed.
  Serial.println("Checking WiFi and updating time if needed...");
  updateTimeIfNeeded(lastWifiCheck, WIFI_CHECK_INTERVAL, rtc, timeClient, ssid, password);

  // Read current time from RTC.
  DateTime now = rtc.now();
  Serial.print("Current RTC time: ");
  Serial.print((now.hour()+2)%12); // Adjust for local time zone
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());

  // Read sensor values on channels 0 and 1.
  int16_t sensorValue1 = ads.readADC_SingleEnded(0);
  int16_t sensorValue2 = ads.readADC_SingleEnded(1);
  Serial.print("Sensor reading channel 0: ");
  Serial.println(sensorValue1);
  Serial.print("Sensor reading channel 1: ");
  Serial.println(sensorValue2);

  // Convert sensor readings to distances.
  float distance1 = calibrateSensor(sensorValue1);
  float distance2 = calibrateSensor(sensorValue2);
  Serial.print("Calculated distances (cm) - Sensor 1: ");
  Serial.print(distance1);
  Serial.print(", Sensor 2: ");
  Serial.println(distance2);

  // Debounced presence detection using a state machine.
  // currentState: false = absence, true = presence.
  static bool currentState = false;
  // stateChangeTime records when a potential state change began.
  static unsigned long stateChangeTime = millis();
  
  // Determine current sensor condition.
  bool sensorPresence = (distance1 > THRESHOLD_DISTANCE || distance2 > THRESHOLD_DISTANCE);
  
  // If sensor condition differs from the current state, check debounce time.
  if (sensorPresence != currentState) {
    if (millis() - stateChangeTime >= 300) { // 100ms debounce period
      currentState = sensorPresence;
      Serial.println("State change confirmed.");
      stateChangeTime = millis();  // Reset timer after state change.
    }
  } else {
    // If condition is stable, reset the timer.
    stateChangeTime = millis();
  }
  
  Serial.print("Current presence state: ");
  Serial.println(currentState ? "PRESENT" : "ABSENT");

  // Update display based on the debounced state.
  if (currentState) {                                   // presence
    Serial.println("Presence confirmed: displaying clock.");
    DisplayHandler::showClock(now);
  } else {                                              // absence
    switch (standbyOverride) {
      case StandbyOverride::Red:
        Serial.println("Absence + red override: solid red.");
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        break;
      case StandbyOverride::Purple:
        Serial.println("Absence + purple override: solid purple.");
        fill_solid(leds, NUM_LEDS, CRGB::Purple);       // or CRGB(128,0,128)
        break;
      case StandbyOverride::None:
      default:
        Serial.println("Absence: normal rainbow fade.");
        DisplayHandler::showFullFade(255);
        return FastLED.show();                          // already done inside
    }
    FastLED.setBrightness(255);
    FastLED.show();
  }

}
