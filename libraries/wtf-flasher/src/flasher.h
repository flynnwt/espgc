#pragma once
#include <Arduino.h>

class Flasher {

  unsigned int pin;
  unsigned long onMs;
  unsigned long offMs;
  bool state;
  bool paused;
  unsigned int num;
  unsigned long changeTime;

  void toggleState(unsigned long nowTime);

public:
  typedef struct {
    unsigned int pin;
    unsigned long on;
    unsigned long off;
  } Config;

  Flasher(unsigned int pin, unsigned long onMs, unsigned long offMs);
  Flasher(Config config);
  Flasher(unsigned int pin);
  void set(bool state);
  void rate(unsigned long onMs, unsigned long offMs);
  void stop();
  void start();
  void once();
  void times(unsigned int);
  void process();
};