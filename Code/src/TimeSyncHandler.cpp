#include "TimeSyncHandler.h"
#include <ESP8266WiFi.h>
#include <RTClib.h>

void updateTimeIfNeeded(unsigned long &lastWifiCheck,
                        unsigned long interval,
                        RTC_DS3231 &rtc,
                        NTPClient &timeClient,
                        const char* ssid,
                        const char* password) {
  if (millis() - lastWifiCheck >= interval) {
    lastWifiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected, attempting reconnect...");
      WiFi.begin(ssid, password);
    } else {
      Serial.println("WiFi connected, updating time from NTP...");
      timeClient.begin();
      timeClient.update();
      rtc.adjust(DateTime(timeClient.getEpochTime()));
    }
  }
}
