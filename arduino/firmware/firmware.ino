#include <PacketSerial.h>

#include<Arduino.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_PIN 6
#define LED_OFFSET 0
#define NUM_LEDS 1 + 24 + 64 +64 + 24
#define TOTAL_NUM_LEDS NUM_LEDS+LED_OFFSET

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel (TOTAL_NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

PacketSerial packetSerial;

void setup() {
  strip.begin();
  strip.setBrightness(96);
  strip.clear();
  strip.show();

  packetSerial.begin(115200);
  packetSerial.setPacketHandler(&onPacketReceived);
}

void loop() {
  packetSerial.update();

  if (packetSerial.overflow())
  {
  }
  strip.show();
}

void onPacketReceived(const uint8_t* buffer, size_t size)
{
  strip.setPixelColor(buffer[0] + LED_OFFSET, buffer[1], buffer[2], buffer[3]);
}
