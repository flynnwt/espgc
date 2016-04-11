#include "status.h"

void Status::init() {
  int i, j;

  for (i = 0; i < NUM_FLASHERS; i++) {
    flashers[i] = 0;
  }
  for (i = 0; i < NUM_STATES; i++) {
    for (j = 0; j < NUM_FLASHERS; j++) {
      flashStates[i][j][0] = 0;
      flashStates[i][j][1] = 0;
    }
  }

}

Status::Status() {
  init();
}

Status::Status(unsigned int pin) {
  init();
  flashers[0] = new Flasher(pin);
}

Status::Status(unsigned int pin[], unsigned int len) {
  int i;

  init();
  if (len > NUM_FLASHERS) {
    len = NUM_FLASHERS;
  }
  for (i = 0; i < len; i++) {
    flashers[i] = new Flasher(pin[i]);
  }
}

Status::Status(Flasher::Config config) {
  flashers[0] = new Flasher(config);
}

// darrr..cpp still can't do array len and don't feel like using vector
Status::Status(Flasher::Config config[], unsigned int len) {
  int i;

  if (len > NUM_FLASHERS) {
    len = NUM_FLASHERS;
  }
  for (i = 0; i < len; i++) {
    flashers[i] = new Flasher(config[i]);
  }
}

void Status::defineState(State s, unsigned long on, unsigned long off) {
  flashStates[(int)s][0][0] = on;
  flashStates[(int)s][0][1] = off;
}

void Status::defineState(State s, unsigned long onOff[][2], unsigned int len) {
  int i;

  if (len > NUM_FLASHERS) {
    len = NUM_FLASHERS;
  }
  for (i = 0; i < len; i++) {
    flashStates[(int)s][i][0] = onOff[i][0];
    flashStates[(int)s][i][1] = onOff[i][1];
  }
}

void Status::process() {
  int i;

  for (i = 0; i < NUM_FLASHERS; i++) {
    if (flashers[i]) {
      flashers[i]->process();
    }
  }
}

void Status::set(State s) {
  int i;

  for (i = 0; i < NUM_FLASHERS; i++) {
    if (flashers[i]) {
      flashers[i]->set(0);
      flashers[i]->rate(flashStates[(int)s][i][0], flashStates[(int)s][i][1]);
    }
  }


}

void Status::set(bool state) {
  int i;

  for (i = 0; i < NUM_FLASHERS; i++) {
    if (flashers[i]) {
      flashers[i]->set(state);
    }
  }
}

void Status::start() {
  int i;

  for (i = 0; i < NUM_FLASHERS; i++) {
    if (flashers[i]) {
      flashers[i]->start();
    }
  }
}

void Status::stop() {
  int i;

  for (i = 0; i < NUM_FLASHERS; i++) {
    if (flashers[i]) {
      flashers[i]->stop();
    }
  }
}
