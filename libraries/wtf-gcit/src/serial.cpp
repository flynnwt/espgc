#include "serial.h"

ConnectorSerial::ConnectorSerial(Connector *c) {
  ConnectorSerial(c, UNDEFINED_SPIN, UNDEFINED_SPIN);
}

ConnectorSerial::ConnectorSerial(Connector *c, int rxSpin, int txSPin) {
  gc = c->parent->parent;
  parent = c;
  this->parity = Parity::none;
  this->flowControl = FlowControl::none;
  this->baudRate = 115200;
  this->recvBuffer = NULL;
  setBufferSize(512);
  //this->enableTcp = false;
  startTcpServer();
}

void ConnectorSerial::set(unsigned int baudRate) {
  this->baudRate = baudRate;
}

void ConnectorSerial::set(unsigned int baudRate, FlowControl flowControl, Parity parity) {
  this->baudRate = baudRate;
  this->flowControl = flowControl;
  this->parity = parity;
}

void ConnectorSerial::setBufferSize(unsigned int size) {
  recvBufferSize = size;
  if (recvBuffer) {
    free(recvBuffer);
  }
  recvBuffer = (char *)malloc(sizeof(char) * recvBufferSize);
  if (!recvBuffer) {
    recvBufferSize = 0;
  }
  recvBufferLen = 0;
  recvBuffer[0] = '\0';
  recvBufferOFlow = false;
}

unsigned int ConnectorSerial::getBaudRate() {
  return baudRate;
}

ConnectorSerial::FlowControl ConnectorSerial::getFlowControl() {
  return flowControl;
}

ConnectorSerial::Parity ConnectorSerial::getParity() {
  return parity;
}

// gc does all serial through tcp connection(s)
// also allow sending on serial through http (api)
// and allow receiving on serial through http (api) by using
//  callback to notify on receives (buffer there and wait for a read, or use websockets, etc.)

void ConnectorSerial::startTcpServer() {
  int i;

  gc->printf("starttcp\n");
  for (i = 0; i < MAX_TCP_CLIENTS; i++) {
    serverClientActive[i] = false;
  }
  enableTcp = true;
  server = new WiFiServer(parent->parent->parent->settings()->serialTcpPort);
  server->setNoDelay(true);
  server->begin();
}

void ConnectorSerial::send(String s) {
  Serial.print(s);
}

void ConnectorSerial::send(char buffer[], unsigned int len) {
  unsigned int i;

  for (i = 0; i < len; i++) {
    Serial.write(buffer[i]);
  }
  Serial.flush();
}

String ConnectorSerial::fixup(char *buffer, unsigned int len) {
  int i;
  char value, hexbuf[2];
  String res = "";

  for (i = 0; i < len; i++) {
    value = buffer[i];
    if ((value < 32) || (value > 126)) {
      sprintf(hexbuf, "%02X", value);
      res += "\\" + String(hexbuf);
    } else if (value == '\\') {
      res += "\\\\";
    } else {
      res += String(value);
    }
  }
  return res;
}

// **************************************************************************************
// Serial Connector Device and TCP

void ConnectorSerial::process() {
  int i, j;
  bool ok = false;
  String req;
  char buffer[128];
  unsigned int len = 0;

  // check for device data received
  // recvBuffer (api): should it oflow or block when full? oflow for now (safer)
  while ((Serial.available() > 0) && (recvBufferLen < recvBufferSize) && (len < 128)) {
    buffer[len] = Serial.read();
    recvBuffer[recvBufferLen++] = buffer[len++];
  }
  while ((Serial.available() > 0) && (recvBufferLen == recvBufferSize) && (len < 128)) {
    buffer[len++] = Serial.read();
    recvBufferOFlow = true;
  }
  if (len > 0) {
    if (enableTcp) {
      for (i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (serverClientActive[i]) {
          for (j = 0; j < len; j++) {
            serverClient[i].print(buffer[j]);
          }
          serverClient[i].flush();
        }
      }
    }
  }

  if (enableTcp) {

    // check for tcp connects
    if (server->hasClient()) {
      for (i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (serverClientActive[i] && !serverClient[i].connected()) {
          gc->printf("SERIALTCP: stopping old client %i\n", i);
          serverClient[i].stop();
          gc->printf("SERIALTCP: replacing with new client\n");
          serverClient[i] = server->available();
          ok = true;
          break;
        } else if (!serverClientActive[i]) {
          gc->printf("SERIALTCP: inserting client %i\n", i);
          serverClient[i] = server->available();
          serverClientActive[i] = true;
          ok = true;
          break;
        }
      }
      // no free spot
      if (!ok) {
        gc->printf("TCP: ignoring new client!\n");
        WiFiClient serverClient = server->available();
        serverClient.stop();
      }
    }

    // check for tcp receives - supposed to read as one tcp packet
    for (i = 0; i < MAX_TCP_CLIENTS; i++) {
      if (serverClientActive[i] && serverClient[i].connected()) {
        if (serverClient[i].available()) {
          serverClient[i].setTimeout(0);
          len = serverClient[i].readBytes(buffer, 128);
          gc->printf("SERIALTCP request(%i): %i bytes, %s\n", i, len, fixup(buffer, len).c_str());
          send(buffer, len);
        }
      } else if (serverClientActive[i] && !serverClient[i].connected()) {
        gc->printf("SERIALTCP: deleting disconnected client %i\n", i);
        serverClient[i].stop();
        serverClientActive[i] = false;
      }
    }
  }
}
