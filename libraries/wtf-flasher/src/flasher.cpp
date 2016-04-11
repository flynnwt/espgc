#include "flasher.h"

Flasher::Flasher(unsigned int pin) {
  this->pin = pin;
  this->onMs = 0;
  this->offMs = 0;
  this->paused = true;
  this->num = 0;
  this->state = 0;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, this->state);
}

Flasher::Flasher(unsigned int pin, unsigned long onMs, unsigned long offMs) {
  this->pin = pin;
  this->onMs = onMs;
  this->offMs = offMs;
  this->paused = true;
  this->num = 0;
  this->state = 0;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, this->state);
}

Flasher::Flasher(Config config) {
  this->pin = config.pin;
  this->onMs = config.on;
  this->offMs = config.off;
  this->paused = true;
  this->num = 0;
  this->state = 0;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, this->state);
}

void Flasher::rate(unsigned long on, unsigned long off) {
  this->onMs = on;
  this->offMs = off;
}

// don't toggle state if opposite state ms is set to 0; allows solid on/off based
//  on rates in addition to set()
void Flasher::toggleState(unsigned long nowTime) {
  if ((state && (offMs == 0)) || (!state && (onMs == 0))) {
    return;
  }
  changeTime = nowTime;
  state = !state;
  digitalWrite(pin, state);
  if (!state) {
    if (num > 1) {
      num--;
    } else if (num == 1) {
      num = 0;
      paused = true;
    }
  }
}

// set changeTime in future?
void Flasher::set(bool s) {
  state = s;
  digitalWrite(pin, state);
}

void Flasher::stop() { paused = true; }

void Flasher::start() {
  paused = false;
  changeTime = millis();
}

void Flasher::once() { times(1); }

void Flasher::times(unsigned int t) {
  num = t;
  set(1);
  start();
}

void Flasher::process() {
  unsigned long nowTime = millis();
  if (!paused && state && (nowTime - changeTime >= onMs)) {
    toggleState(nowTime);
  } else if (!paused && !state && (nowTime - changeTime >= offMs)) {
    toggleState(nowTime);
  }
}