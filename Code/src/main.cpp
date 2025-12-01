#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RTClib.h>
#include <Adafruit_ADS1X15.h>
#include <FastLED.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "DisplayHandler.h"
#include "SensorHandler.h"
#include "TimeSyncHandler.h"

// WiFi credentials
const char* ssid     = "Cat";
const char* password = "ciao12345";

// Firebase Realtime Database endpoint
const char* FIREBASE_STATUS_URL =
  "https://workstatus-5a293-default-rtdb.europe-west1.firebasedatabase.app/indicator/status.json";

// LED configuration
const int NUM_LEDS = 60;
#define LED_PIN D6
CRGB leds[NUM_LEDS]; // Defined so it's accessible by DisplayHandler.

RTC_DS3231 rtc;
Adafruit_ADS1115 ads;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // NTP update interval: 60 sec

unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 100000;  // 100 sec

// Rimane anche se non la usi più per logica
const float THRESHOLD_DISTANCE = -3000;

// Stato remoto letto da Firebase
String remoteStatus = "default";
unsigned long lastStatusFetch = 0;
const unsigned long STATUS_FETCH_INTERVAL = 5000;  // 5 s

// Override standby via seriale (fallback quando non c'è connessione)
enum class StandbyOverride { None, Red, Purple, Green, Orange, Clock };
StandbyOverride standbyOverride = StandbyOverride::None;

// ─────────────────────────────────────
// FUNZIONI PER FIREBASE
// ─────────────────────────────────────

String fetchRemoteStatusOnce() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi not connected, skip Firebase fetch."));
    return "";
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // accetta qualsiasi certificato (ok per uso interno)

  HTTPClient https;
  Serial.print(F("Fetching status from Firebase: "));
  Serial.println(FIREBASE_STATUS_URL);

  if (!https.begin(client, FIREBASE_STATUS_URL)) {
    Serial.println(F("HTTPS begin() failed"));
    return "";
  }

  int httpCode = https.GET();
  if (httpCode != 200) {
    Serial.print(F("HTTPS GET failed, code = "));
    Serial.println(httpCode);
    https.end();
    return "";
  }

  String payload = https.getString();
  https.end();

  payload.trim();
  Serial.print(F("Raw Firebase payload: "));
  Serial.println(payload);

  // payload atteso: "default", "lab", "busy", ...
  if (payload.length() >= 2 && payload[0] == '"' && payload[payload.length() - 1] == '"') {
    payload = payload.substring(1, payload.length() - 1);
  }
  payload.trim();

  Serial.print(F("Parsed Firebase status: "));
  Serial.println(payload);
  return payload;
}

void updateRemoteStatus() {
  unsigned long nowMillis = millis();
  if (nowMillis - lastStatusFetch < STATUS_FETCH_INTERVAL) {
    return; // non refetcha troppo spesso
  }
  lastStatusFetch = nowMillis;

  String s = fetchRemoteStatusOnce();
  if (s.length() == 0) {
    // niente update, tengo l’ultimo valido
    return;
  }

  // accetto solo gli stati che conosciamo
  if (s == "default" || s == "lab" || s == "busy" ||
      s == "free" || s == "away" || s == "x") {
    remoteStatus = s;
  } else {
    Serial.print(F("Unknown status from Firebase, ignoring: "));
    Serial.println(s);
  }
}

// ─────────────────────────────────────
// SETUP
// ─────────────────────────────────────

void setup() {
  Serial.begin(115200);
  Serial.println("=== System Initialization ===");
  
  // Initialize LED strip.
  Serial.println("Initializing LED strip...");
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  DisplayHandler::showFullFade(255);
  FastLED.show();
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
    FastLED.show();
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
  FastLED.show();
  lastWifiCheck = millis();
  Serial.println("Setup complete.");
}

// ─────────────────────────────────────
// LOOP
// ─────────────────────────────────────

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

  // ── PARSING SERIALE (fallback offline) ──
  while (Serial.available() > 0) {
    char c = Serial.read();
    switch (c) {
      case 'c': case 'C':
        standbyOverride = StandbyOverride::Clock;
        Serial.println(F("Standby override → CLOCK DISPLAY"));
        break;
      case 'g': case 'G':
        standbyOverride = StandbyOverride::Green;
        Serial.println(F("Standby override → GREEN"));
        break;
      case 'o': case 'O':
        standbyOverride = StandbyOverride::Orange;
        Serial.println(F("Standby override → ORANGE"));
        break;
      case 'r': case 'R':
        standbyOverride = StandbyOverride::Red;
        Serial.println(F("Standby override → RED"));
        break;
      case 'p': case 'P':
        standbyOverride = StandbyOverride::Purple;
        Serial.println(F("Standby override → PURPLE"));
        break;
      case 'n': case 'N':
        standbyOverride = StandbyOverride::None;
        Serial.println(F("Standby override → NORMAL FADE"));
        break;
    }
  }

  // Update WiFi/NTP time if needed.
  Serial.println("Checking WiFi and updating time if needed...");
  updateTimeIfNeeded(lastWifiCheck, WIFI_CHECK_INTERVAL, rtc, timeClient, ssid, password);

  // Se WiFi è connesso → prova ad aggiornare stato remoto
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Updating remote status from Firebase (if needed)...");
    updateRemoteStatus();
  } else {
    Serial.println("WiFi disconnected → using serial override mode.");
  }

  // Read current time from RTC.
  DateTime now = rtc.now();
  Serial.print("Current RTC time: ");
  Serial.print((now.hour()+2)%12); // Adjust for local time zone
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());

  // Read sensor values on channels 0 and 1. (solo logging)
  int16_t sensorValue1 = ads.readADC_SingleEnded(0);
  int16_t sensorValue2 = ads.readADC_SingleEnded(1);
  Serial.print("Sensor reading channel 0: ");
  Serial.println(sensorValue1);
  Serial.print("Sensor reading channel 1: ");
  Serial.println(sensorValue2);

  float distance1 = calibrateSensor(sensorValue1);
  float distance2 = calibrateSensor(sensorValue2);
  Serial.print("Calculated distances (cm) - Sensor 1: ");
  Serial.print(distance1);
  Serial.print(", Sensor 2: ");
  Serial.println(distance2);

  // ─────────────────────────────────────
  // DISPLAY LOGIC
  // ─────────────────────────────────────

  if (WiFi.status() == WL_CONNECTED) {
    // ONLINE: usa sempre lo stato remoto
    Serial.print("ONLINE mode, remoteStatus = ");
    Serial.println(remoteStatus);

    if (remoteStatus == "away") {
      Serial.println("Status = away → displaying clock.");
      DisplayHandler::showClock(now);
      FastLED.setBrightness(255);
      FastLED.show();
    } else if (remoteStatus == "lab") {
      Serial.println("Status = lab → solid orange.");
      fill_solid(leds, NUM_LEDS, CRGB::Orange);
      FastLED.setBrightness(255);
      FastLED.show();
    } else if (remoteStatus == "busy") {
      Serial.println("Status = busy → solid red.");
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      FastLED.setBrightness(255);
      FastLED.show();
    } else if (remoteStatus == "free") {
      Serial.println("Status = free → solid green.");
      fill_solid(leds, NUM_LEDS, CRGB::Green);
      FastLED.setBrightness(255);
      FastLED.show();
    } else if (remoteStatus == "x") {
      Serial.println("Status = x (Lab MRI) → solid purple.");
      fill_solid(leds, NUM_LEDS, CRGB::Purple);
      FastLED.setBrightness(255);
      FastLED.show();
    } else {
      // default o sconosciuto → rainbow fade
      Serial.println("Status = default/unknown → rainbow fade.");
      DisplayHandler::showFullFade(255);
      FastLED.setBrightness(255);
      FastLED.show();
    }

  } else {
    // OFFLINE: usa override da seriale come prima
    Serial.println("OFFLINE mode → using serial standbyOverride");

    switch (standbyOverride) {
      case StandbyOverride::Clock:
        Serial.println("Offline override: displaying clock.");
        DisplayHandler::showClock(now);
        FastLED.setBrightness(255);
        FastLED.show();
        break;
      case StandbyOverride::Green:
        Serial.println("Offline override: solid green.");
        fill_solid(leds, NUM_LEDS, CRGB::Green);
        FastLED.setBrightness(255);
        FastLED.show();
        break;
      case StandbyOverride::Red:
        Serial.println("Offline override: solid red.");
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.setBrightness(255);
        FastLED.show();
        break;
      case StandbyOverride::Purple:
        Serial.println("Offline override: solid purple.");
        fill_solid(leds, NUM_LEDS, CRGB::Purple);
        FastLED.setBrightness(255);
        FastLED.show();
        break;
      case StandbyOverride::Orange:
        Serial.println("Offline override: solid orange.");
        fill_solid(leds, NUM_LEDS, CRGB::Orange);
        FastLED.setBrightness(255);
        FastLED.show();
        break;
      case StandbyOverride::None:
      default:
        Serial.println("Offline override: normal rainbow fade.");
        DisplayHandler::showFullFade(255);
        FastLED.setBrightness(255);
        FastLED.show();
        break;
    }
  }
}
