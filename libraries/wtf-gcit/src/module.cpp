#include "module.h"

// for true gc, must have 1 eth/wifi and 1 ir/serial/relay
Module::Module(GCIT *p, unsigned int a, ModuleType t) {
  int i;

  parent = p;
  type = t;
  address = a;
  numConnectors = 0;
  for (i = 0; i < MODULE_MAX_CONNECTORS; i++) {
    connectors[i] = NULL;
  }
  switch (type) {
  case ModuleType::ethernet:
    p->settings()->model = "ITachIP2";
    descriptor = "0 ETHERNET";
    break;
  case ModuleType::wifi:
    p->settings()->model = "ITachWF2";
    descriptor = "0 WIFI";
    break;
  case ModuleType::ir:
    p->settings()->model += "IR";
    descriptor = "0 IR";
    break;
  case ModuleType::serial:
    p->settings()->model += "SL";
    descriptor = "0 SERIAL";
    break;
  case ModuleType::relay:
    p->settings()->model += "CC";
    descriptor = "0 RELAY";
    break;
  default:
    p->settings()->model = "UNKNOWN";
    descriptor = "0 UNKNOWN";
  }
}

int Module::addConnector(ConnectorType t) { return addConnector(t, -1, -1); }

int Module::addConnector(ConnectorType t, int fPin) { return addConnector(t, fPin, -1); }

int Module::addConnector(ConnectorType t, int fPin, int sPin) { return addConnector(t, numConnectors + 1, fPin, sPin); }

int Module::addConnector(ConnectorType t, int address, int fPin, int sPin) {

  if (address > MODULE_MAX_CONNECTORS) {
    return 0;
  }

  connectors[address - 1] = new Connector(this, address, t, fPin, sPin);
  numConnectors++;
  switch (type) {
  case ModuleType::ethernet:
  case ModuleType::wifi:
    // ERROR!
    break;
  case ModuleType::ir:
    descriptor = String(numConnectors) + " IR";
    break;
  case ModuleType::serial:
    descriptor = String(numConnectors) + " SERIAL";
    break;
  case ModuleType::relay:
    descriptor = String(numConnectors) + " RELAY";
    break;
  default:
    descriptor = String(numConnectors) + " UNKNOWN";
    break;
  }

  return 1;
}

// connector numbering starts at 1!!!
Connector *Module::getConnector(int address) {
  if (address == 0) {
    return NULL;
    //} else if (c > numConnectors) {
    //  return NULL;
  } else {
    return connectors[address - 1];
  }
}