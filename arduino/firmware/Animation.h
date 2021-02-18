#pragma once
#include "./defines.h"
#include <Adafruit_NeoPixel.h>
#include <Array.h>

class Pixel {
  public:
    Pixel(){}
    Pixel(byte index, byte red, byte green, byte blue) : _index(index), _red(red), _green(green), _blue(blue) {}
    
    byte _index;
    byte _red;
    byte _green;
    byte _blue;
};

class Frame {
  public:
    Frame(){}
    Frame(int timestamp) : _timestamp(timestamp) {}

    int getTimestamp() {
      return _timestamp;
    }
    void addPixel(byte index, byte red, byte green, byte blue) {
      _pixels.push_back(Pixel(index, red, green, blue));
    }
    
    unsigned int _timestamp;
    Array<Pixel, 30> _pixels;
};

template <size_t MAXNUMBEROFFRAMES>
class Animation {
  public:
    void trigger(Adafruit_NeoPixel strip) {
      unsigned int i = 0;
      for (auto frame : _frames) {
        Serial.println(frame.getTimestamp());
        for (auto pixel : frame._pixels) {
          strip.setPixelColor(pixel._index, pixel._red, pixel._green, pixel._blue);
        }
        strip.show();
        if (i < _frames.size() - 1) {
          delay(_frames[i + 1].getTimestamp() - frame.getTimestamp());
        }
        i++;
      }
    }
    void addNewFrame(int timestamp) {
      _frames.push_back(Frame(timestamp));
    }
    void addPixelToLastFrame(int index, int red, int green, int blue) {
      if (!_frames.empty()) {
        _frames[_frames.size() - 1].addPixel(index, red, green, blue);
      }
    }
    Array<Frame, MAXNUMBEROFFRAMES> _frames;
};
