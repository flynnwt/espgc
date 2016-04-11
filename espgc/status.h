#pragma once

#include <flasher.h>

#define NUM_FLASHERS 3
#define NUM_STATES 5

// translate system states to status indicator(s)
//
// use with one led, multiple leds, rgb led
//
// init with flasher config and control all with start/stop/set
// or
// define/set states to turn leds on/off/flash individually and together

class Status {
public:
  enum State { error, reset, boot, ap, wifi };

private:
  Flasher *flashers[NUM_FLASHERS];
  unsigned long flashStates[NUM_STATES][NUM_FLASHERS][2];
  void init();
  State state;

public:
  //enum State { error, reset, boot, ap, wifi };

  Status();
  Status(Flasher::Config config);
  Status(Flasher::Config config[], unsigned int len);
  Status(unsigned int pin);
  Status(unsigned int pin[], unsigned int len);

  void defineState(State s, unsigned long on, unsigned long off);
  void defineState(State s, unsigned long onOff[][2], unsigned int len);

  void process();
  void set(State s);
  void set(bool state);
  void start();
  void stop();
};