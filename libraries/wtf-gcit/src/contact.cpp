#include "contact.h"

// call all sensorNotify's to handle timer and state change check
void GCIT::checkSensors() {
  int i, j;
  Module *m;
  Connector *c;

  for (i = 0; i < MAX_MODULES; i++) {
    m = modules[i];
    if (m) {
      for (j = 1; j <= MODULE_MAX_CONNECTORS; j++) { // connectors start at 1!
        c = m->getConnector(j);
        if (c) {
          if (c->type == ConnectorType::sensorNotify) {
            ((ConnectorSensorNotify *)c->control)->update();
          }
        }
      }
    }
  }
}

// Sensor Connector
// allow fcnPin=17, to use A0 analog read

ConnectorSensor::ConnectorSensor(Connector *c, int fPin, int sPin) {
  parent = c;
  fcnPin = fPin;
  statusPin = sPin;
  if (fcnPin != UNDEFINED_FPIN) {
    if (fcnPin == A0) {
      pinMode(fcnPin, INPUT);
    } else if (fcnPin != 16) {
      pinMode(fcnPin, INPUT_PULLUP);
    } else {
      pinMode(fcnPin, INPUT_PULLDOWN_16);
    }
  }
  if (statusPin != UNDEFINED_SPIN) {
    pinMode(statusPin, OUTPUT);
  }
  if (fcnPin == A0) {
    state = analogRead(fcnPin);
  } else {
    state = digitalRead(fcnPin);
  }
}

unsigned int ConnectorSensor::getState() {
  if (fcnPin == A0) {
    state = analogRead(fcnPin);
  } else {
    state = digitalRead(fcnPin);
  }
  flashStatus();
  return state;
}

// Sensor-Notify Connector

ConnectorSensorNotify::ConnectorSensorNotify(Connector *c, int fPin, int sPin) {
  parent = c;
  timer = millis();
  interval = 60000;
  fcnPin = fPin;
  statusPin = sPin;
  if (fcnPin != UNDEFINED_FPIN) {
    if (fcnPin == A0) {
      pinMode(fcnPin, INPUT);
    } else if (fcnPin != 16) {
      pinMode(fcnPin, INPUT_PULLUP);
    } else {
      pinMode(fcnPin, INPUT_PULLDOWN_16);
    }
  }
  if (statusPin != UNDEFINED_SPIN) {
    pinMode(statusPin, OUTPUT);
  }
  if (fcnPin == A0) {
    state = analogRead(fcnPin);
  } else {
    state = digitalRead(fcnPin);
  }
}

void ConnectorSensorNotify::setInterval(unsigned long ms) {
  interval = ms;
}

void ConnectorSensorNotify::update() {
  unsigned int prevState = state;

  if (fcnPin != UNDEFINED_FPIN) {
    if (fcnPin == A0) {
      state = analogRead(fcnPin);
    } else {
      state = digitalRead(fcnPin);
    }

    if ((state != prevState) || ((interval != 0) && (millis() >= timer + interval))) {
      parent->parent->parent->sensorNotify(this); // connector control->connector->module->gc; callback would be nicer
      timer = millis();
    }
  }
}

// Relay Connector
// state is logical state; negative fcnPin inverts physical reads/writes

ConnectorRelay::ConnectorRelay(Connector *c, int fPin, int sPin) {
  parent = c;
  fcnPin = fPin;
  statusPin = sPin;
  state = 0;
  if (fcnPin != UNDEFINED_FPIN) {
    if (fcnPin > 0) {
      pinMode(fcnPin, OUTPUT);
      digitalWrite(fcnPin, state);
    } else {
      pinMode(-fcnPin, OUTPUT);
      digitalWrite(-fcnPin, 1 - state);
    }
  }
  if (statusPin != UNDEFINED_SPIN) {
    pinMode(statusPin, OUTPUT);
  }
}

void ConnectorRelay::setState(unsigned int s) {
  if (s != 0) {
    state = 1;
  } else {
    state = 0;
  }
  if (fcnPin != UNDEFINED_FPIN) {
    if (fcnPin > 0) {
      digitalWrite(fcnPin, state);
    } else {
      digitalWrite(-fcnPin, 1 - state);
    }
  }
}

unsigned int ConnectorRelay::getState() {
  flashStatus();
  return state;
}
