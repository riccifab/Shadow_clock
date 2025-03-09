#ifndef TIMESYNCHANDLER_H
#define TIMESYNCHANDLER_H

#include <RTClib.h>   // Use RTClib which provides the DS3231 support
#include <NTPClient.h>

// Checks Wi‑Fi connectivity and, if connected, updates the RTC using NTP.
// If not connected, attempts to reconnect using the provided credentials.
void updateTimeIfNeeded(unsigned long &lastWifiCheck,
                        unsigned long interval,
                        RTC_DS3231 &rtc,
                        NTPClient &timeClient,
                        const char* ssid,
                        const char* password);

#endif
