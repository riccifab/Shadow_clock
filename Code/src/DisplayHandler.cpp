#include "DisplayHandler.h"

uint8_t hue = 0;        // Global hue variable

// ── helper: rotate AND mirror so the dial runs clockwise ────────────────────
namespace {
  // 60-pixel ring → 30 is the physical “top”.  
  // We want minute 0 at LED 30 and minute numbers to *decrease* when the dial
  // moves clockwise, so we subtract the logical index instead of adding it.
  constexpr int LED_COUNT  = 60;
  constexpr int LED_OFFSET = LED_COUNT / 2;          // 30
  inline int rot(int idx) {                          // clockwise correction
    return (LED_OFFSET - idx + LED_COUNT) % LED_COUNT;
  }
}

// ───────────────────────────── showClock ────────────────────────────────────
void DisplayHandler::showClock(const DateTime &now)
{
  const int second = now.second();           // 0-59
  const int minute = now.minute();           // 0-59
  const int hour   = (now.hour() + 2) % 12;  // local time, 0-11

  FastLED.clear();

  const int hourLed = (hour * 5 + minute / 12) % NUM_LEDS;

  leds[ rot(minute) ] = CRGB::Blue;          // minute
  if (minute == hourLed)
       leds[ rot(minute) ] = CRGB::Magenta;  // overlap
  else leds[ rot(hourLed) ] = CRGB::Red;     // hour

  leds[ rot(second) ] = CRGB::White;         // seconds (always on)

  FastLED.setBrightness(255);
  FastLED.show();
}

// ─────────────────────────── showFullFade ───────────────────────────────────
void DisplayHandler::showFullFade(uint8_t brightness)
{
  static int currentLED = 0;
  FastLED.setBrightness(brightness);

  leds[currentLED] = CHSV(hue, 255, 255);
  hue++;
  currentLED = (currentLED + 1) % NUM_LEDS;

  FastLED.show();
}
