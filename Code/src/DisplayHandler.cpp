#include "DisplayHandler.h"

void DisplayHandler::showClock(const DateTime &now) {
  int minute = now.minute();
  int hour = now.hour() % 12; // convert to 12-hour format

  // Calculate hour LED position on a 60-LED circle.
  // Each hour covers roughly 5 LED positions.
  int hourLed = (hour * 5 + minute / 12) % NUM_LEDS;

  FastLED.clear();

  // Set minute indicator to blue.
  leds[minute] = CRGB::Blue;
  
  // Set hour indicator to red (if same LED, blend to magenta).
  if (minute == hourLed) {
    leds[minute] = CRGB::Magenta;
  } else {
    leds[hourLed] = CRGB::Red;
  }
  
  FastLED.setBrightness(255);
  FastLED.show();
}

void DisplayHandler::showFullFade(uint8_t brightness) {
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
}
