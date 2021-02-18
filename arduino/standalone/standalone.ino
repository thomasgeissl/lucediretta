#include "./defines.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "./startAnimation.h"

#ifdef __AVR__
#include <avr/power.h>
#endif


Adafruit_NeoPixel strip = Adafruit_NeoPixel (TOTAL_NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(96);
  strip.clear();
  strip.show();

  create_startAnimation();
  startAnimation.trigger(strip);

}

void loop() {
}
