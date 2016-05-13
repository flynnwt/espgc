#include "serial.h"

ConnectorSerial::ConnectorSerial(Connector *c) {
  ConnectorSerial(c, UNDEFINED_SPIN, UNDEFINED_SPIN);
}

ConnectorSerial::ConnectorSerial(Connector *c, int rxSpin, int txSPin) {
  ConnectorControl(c, rxSpin, txSPin);
  this->parity = Parity::none;
  this->flowControl = FlowControl::none;
  this->baudRate = 115200;
  this->enableTcp = false;
}

void ConnectorSerial::set(unsigned int baudRate) {
  this->baudRate = baudRate;
}

void ConnectorSerial::set(unsigned int baudRate, FlowControl flowControl, Parity parity) {
  this->baudRate = baudRate;
  this->flowControl = flowControl;
  this->parity = parity;
}

unsigned int ConnectorSerial::getBaudRate() {
  return this->baudRate;
}

ConnectorSerial::FlowControl ConnectorSerial::getFlowControl() {
  return this->flowControl;
}

ConnectorSerial::Parity ConnectorSerial::getParity() {
  return this->parity;
}

// gc does all serial through tcp connection(s)
// also allow sending on serial through http (api)
// and allow receiving on serial through http (api) by using
//  callback to notify on receives (buffer there and wait for a read, or use websockets, etc.)

void ConnectorSerial::startTcpServer() {
  server = new WiFiServer(parent->parent->parent->settings()->serialTcpPort);
  server->setNoDelay(true);
  server->begin();
  enableTcp = true;
}

void ConnectorSerial::send(String s) {
  Serial.print(s);
}

void ConnectorSerial::process() {
  int i;
  bool ok = false;
  String req;
  String deviceData = "";

  // check for device data received
  while (Serial.available()) {
    deviceData += Serial.readString();
  }
  if (deviceData.length() > 0) {
    if (receiveCB != NULL) {
      (receiveCB)(deviceData);
    }
    if (enableTcp) {
      for (i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (serverClientActive[i]) {
          serverClient[i].print(deviceData);
        }
      }
    }
  }

  // check for tcp receives
  if (enableTcp) {

    if (server->hasClient()) {
      for (i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (serverClientActive[i] && !serverClient[i].connected()) {
          //serial->printf("TCP: stopping old client %i\n", i);
          serverClient[i].stop();
          //serial->println("TCP: replacing with new client");
          serverClient[i] = server->available();
          ok = true;
          break;
        } else if (!serverClientActive[i]) {
          //serial->printf("TCP: inserting client %i\n", i);
          serverClient[i] = server->available();
          serverClientActive[i] = true;
          ok = true;
          break;
        }
      }
      // no free spot - deny or close by lru?
      if (!ok) {
        //serial->println("TCP: ignoring new client!");
        WiFiClient serverClient = server->available();
        serverClient.stop();
      }
    }

    for (i = 0; i < MAX_TCP_CLIENTS; i++) {
      if (serverClientActive[i] && serverClient[i].connected()) {
        if (serverClient[i].available()) {
          req = serverClient[i].readString();
          //serial->printf("TCP request(%i): %s\n", i, fixup(req).c_str());
          send(req);
        }
      } else if (serverClientActive[i] && !serverClient[i].connected()) {
        //serial->printf("TCP: deleting disconnected client %i\n", i);
        serverClient[i].stop();
        serverClientActive[i] = false;
      }
    }
  }
}
