#ifndef DISPLAYHANDLER_H
#define DISPLAYHANDLER_H

#include <RTClib.h>
#include <FastLED.h>

// Declare extern so that the LED array is visible to the handler.
extern CRGB leds[];
extern const int NUM_LEDS;

class DisplayHandler {
public:
  // Displays the clock (hour and minute indicators) on the LED strip.
  static void showClock(const DateTime &now);

  // Fills the LED strip with white at the given brightness.
  static void showFullFade(uint8_t brightness);
};

#endif
