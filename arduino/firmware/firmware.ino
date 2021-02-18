#include "./defines.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PacketSerial.h>

#include "./startAnimation.h"

#ifdef __AVR__
#include <avr/power.h>
#endif


Adafruit_NeoPixel strip = Adafruit_NeoPixel (TOTAL_NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
PacketSerial packetSerial;
bool dirty;

void setup() {
  strip.begin();
  strip.setBrightness(96);
  strip.clear();
  strip.show();

  packetSerial.begin(115200);
  packetSerial.setPacketHandler(&onPacketReceived);

  create_startAnimation();
  startAnimation.trigger(strip);
}

void loop() {
  packetSerial.update();

  if (packetSerial.overflow())
  {
  }
  if (dirty) {
    strip.show();
    dirty = false;
  }
}

void onPacketReceived(const uint8_t* buffer, size_t size)
{
  strip.setPixelColor(buffer[0] + LED_OFFSET, buffer[1], buffer[2], buffer[3]);
  dirty = true;
}
