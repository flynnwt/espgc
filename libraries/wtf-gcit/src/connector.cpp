#include "connector.h"

Connector::Connector(Module *m, unsigned int a, ConnectorType t) { Connector(m, a, t, -1, -1); }

Connector::Connector(Module *m, unsigned int a, ConnectorType t, int fPin) {
  Connector(m, a, t, fPin, -1);
}

Connector::Connector(Module *m, unsigned int a, ConnectorType t, int fPin, int sPin) {
  parent = m;
  address = a;
  type = t;
  switch (type) {
  case ConnectorType::ir:
    descriptor = "IR";
    control = new ConnectorIr(this, fPin, sPin);
    break;
  case ConnectorType::irBlaster:
    descriptor = "IR_BLASTER";
    control = new ConnectorIr(this, fPin, sPin);
    break;
  case ConnectorType::sensor:
    descriptor = "SENSOR";
    control = new ConnectorSensor(this, fPin, sPin);
    break;
  case ConnectorType::sensorNotify:
    descriptor = "SENSOR_NOTIFY";
    control = new ConnectorSensorNotify(this, fPin, sPin);
    break;
  case ConnectorType::ledLighting:
    descriptor = "LED_LIGHTING";
    control = new ConnectorLedLighting(this, fPin, sPin);
    break;
  case ConnectorType::serial:
    descriptor = "SERIAL";
    control = new ConnectorSerial(this, fPin, sPin);
    break;
  case ConnectorType::relay:
    descriptor = "RELAY";
    control = new ConnectorRelay(this, fPin, sPin);
    break;
  default:
    descriptor = "";
    control = NULL;
  }
}

ConnectorControl::ConnectorControl(Connector *c) { ConnectorControl(c, -1, -1); }

ConnectorControl::ConnectorControl(Connector *c, int fPin) { ConnectorControl(c, fPin, -1); }

ConnectorControl::ConnectorControl(Connector *c, int fPin, int sPin) {
  parent = c;
  fcnPin = fPin;
  statusPin = sPin;
  if (statusPin != -1) {
    pinMode(statusPin, OUTPUT);
  }
}

void ConnectorControl::setStatus(unsigned int v) {
  if (statusPin != -1) {
    digitalWrite(statusPin, v);
  }
}

void ConnectorControl::flashStatus() {
  if (statusPin != -1) {
    digitalWrite(statusPin, HIGH);
    delay(100);
    digitalWrite(statusPin, LOW);
  }
}
