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

ConnectorSensor::ConnectorSensor(Connector *c, int fPin, int sPin) {
  parent = c;
  fcnPin = fPin;
  statusPin = sPin;
  if (fcnPin != -1) {
    if (fcnPin != 16) {
      pinMode(fcnPin, INPUT_PULLUP);
    } else {
      pinMode(fcnPin, INPUT_PULLDOWN_16);
    }
  }
  if (statusPin != -1) {
    pinMode(statusPin, OUTPUT);
  }
  state = digitalRead(fcnPin);
}

unsigned int ConnectorSensor::getState() {
  state = digitalRead(fcnPin);
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
  if (fcnPin != -1) {
    if (fcnPin != 16) {
      pinMode(fcnPin, INPUT_PULLUP);
    } else {
      pinMode(fcnPin, INPUT_PULLDOWN_16);
    }
  }
  if (statusPin != -1) {
    pinMode(statusPin, OUTPUT);
  }
  state = digitalRead(fcnPin);
}

void ConnectorSensorNotify::setInterval(unsigned long ms) { interval = ms; }

void ConnectorSensorNotify::update() {
  unsigned int prevState = state;

  if (fcnPin != -1) {
    state = digitalRead(fcnPin);

    if ((state != prevState) || ((interval != 0) && (millis() >= timer + interval))) {
      parent->parent->parent->sensorNotify(this); // connector control->connector->module->gc; callback would be nicer
      timer = millis();
    }
  }
}

// Relay Connector

ConnectorRelay::ConnectorRelay(Connector *c, int fPin, int sPin) {
  parent = c;
  fcnPin = fPin;
  statusPin = sPin;
  if (fcnPin != -1) {
    pinMode(fcnPin, OUTPUT);
  }
  if (statusPin != -1) {
    pinMode(statusPin, OUTPUT);
  }
  state = 0;
  digitalWrite(fcnPin, state);
}

void ConnectorRelay::setState(unsigned int s) {
  state = s;
  digitalWrite(fcnPin, state);
}

unsigned int ConnectorRelay::getState() {
  flashStatus();
  return state;
}
