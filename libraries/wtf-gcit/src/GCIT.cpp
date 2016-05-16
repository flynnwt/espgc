#include "GCIT.h"

void GCIT::init() {
  int i;

  // this DOESN'T WORK! (defaulting to Serial doesn't work when serial is a Log)
  // seems like the only way for it to work is to have serial be dynamic type at compile
  //  (HardwareSerial or Log), but don't know how to get that to happen
  // had to do it this way because if serial is HardwareSerial, the Log methods don't hide
  //  the base class (HardwareSerial) methods because they aren't virtual (I think); so
  //  this lib needs to know about logger anyway (can't just set serial to a HardwareSerial or Log
  //  in user code)
  // only other way is to create a printf() here, always use it, and have it check if
  //  logger is non-NULL to decide serial vs logger
  // serial = (Log *)&Serial;

  serial = &Serial;
  logger = NULL;
  server = NULL;
  state.enableTcp = false;
  state.enableDiscovery = true;

  for (i = 0; i < MAX_TCP_CLIENTS; i++) {
    serverClientActive[i] = false;
  }
  for (i = 0; i < MAX_MODULES; i++) {
    modules[i] = NULL;
  }
  WiFi.macAddress(MAC);
};

GCIT::GCIT() {
  numModules = 0;
  init();
}

GCIT::GCIT(ModuleType t) {
  numModules = 0;
  // if ((t == ModuleType::ethernet) || (t == ModuleType::wifi)) {
  addModule(t);
  init();
  //}
}

// use to select between 'normal' serial output and logger output;
void GCIT::printf(const char *format, ...) {
  size_t len;
  va_list arg;
  char temp[1460];

  va_start(arg, format);
  len = ets_vsnprintf(temp, 1460, format, arg);
  va_end(arg);

  if (logger != NULL) {
    logger->print(temp);
  } else {
    serial->print(temp);
  }
}

ModuleType GCIT::getModuleType(String t) {
  t.toLowerCase();
  if (t == "wifi") {
    return ModuleType::wifi;
  } else if (t == "ethernet") {
    return ModuleType::ethernet;
  } else if (t == "ir") {
    return ModuleType::ir;
  } else if (t == "serial") {
    return ModuleType::serial;
  } else if (t == "relay") {
    return ModuleType::relay;
  } else {
    return (ModuleType)UNDEFINED_MOD;
  }
}

String GCIT::getModuleType(ModuleType t) {
  switch (t) {
  case ModuleType::ethernet:
    return "Ethernet";
    break;
  case ModuleType::wifi:
    return "WiFi";
    break;
  case ModuleType::ir:
    return "IR";
    break;
  case ModuleType::relay:
    return "Relay";
    break;
  case ModuleType::serial:
    return "Serial";
    break;
  default:
    return "UNKNOWN:";
    break;
  }
}

ConnectorType GCIT::getConnectorType(String t) {
  t.toLowerCase();
  if (t == "ir") {
    return ConnectorType::ir;
  } else if (t == "irblaster") {
    return ConnectorType::irBlaster;
  } else if (t == "sensor") {
    return ConnectorType::sensor;
  } else if (t == "sensornotify") {
    return ConnectorType::sensorNotify;
  } else if (t == "ledlighting") {
    return ConnectorType::ledLighting;
  } else if (t == "serial") {
    return ConnectorType::serial;
  } else if (t == "relay") {
    return ConnectorType::relay;
  } else {
    return (ConnectorType)UNDEFINED_CONN;
  }
}

String GCIT::getConnectorType(ConnectorType t) {
  switch (t) {
  case ConnectorType::ir:
    return "IR";
    break;
  case ConnectorType::irBlaster:
    return "IRBlaster";
    break;
  case ConnectorType::sensor:
    return "Sensor";
    break;
  case ConnectorType::sensorNotify:
    return "SensorNotify";
    break;
  case ConnectorType::ledLighting:
    return "LEDLighting";
    break;
  case ConnectorType::serial:
    return "Serial";
    break;
  case ConnectorType::relay:
    return "Relay";
    break;
  default:
    return "UNKNOWN:";
    break;
  }
}

Settings *GCIT::settings() { return &state; }

void GCIT::lock(bool enable) { state.locked = enable; }

void GCIT::enableDiscovery(bool enable) { state.enableDiscovery = enable; }

void GCIT::enableTcp(bool enable) {
  if (enable && !state.enableTcp) {
    startTcpServer();
  } else if (!enable && state.enableTcp) {
    stopTcpServer();
  }
}

void GCIT::enableSerial(ConnectorSerial *c) {
  serialConnector = c;
}

void GCIT::setIpAddr(IPAddress ip) {
  for (int i = 0; i < 4; i++) {
    ipAddr[i] = ip[i];
  }
}

Module *GCIT::addModule(ModuleType t) { return addModule(numModules, t); }

Module *GCIT::addModule(int address, ModuleType t) {
  Module *m = new Module(this, address, t);
  modules[address] = m;
  numModules++;
  return m;
}

Module *GCIT::getModule(unsigned int m) { return modules[m]; }

// connector numbering starts at 1!!!
Connector *GCIT::getConnector(unsigned int m, unsigned int c) {
  Module *mod = getModule(m);
  if (mod == NULL) {
    return NULL;
  } else {
    return mod->getConnector(c);
  }
}

void GCIT::startTcpServer() { startTcpServer(state.tcpPort); }

void GCIT::startTcpServer(int port) {
  state.tcpPort = port;
  state.enableTcp = true;
  server = new WiFiServer(state.tcpPort);
  server->setNoDelay(true);
  server->begin();
  return;
}

void GCIT::stopTcpServer() {
  state.enableTcp = false;
  if (server) {
    delete server; // i guess?!?
    server = NULL;
  }
}

void GCIT::process() {
  discovery();
  checkSensors();
  handleClient();
  if (serialConnector) {
    serialConnector->process();
  }
}

/*
sensor notify broadcast (9132)
*/

void GCIT::sensorNotify(ConnectorSensorNotify *c) {
  if (state.enableTcp) {
    String res = "sensornotify," + String(c->parent->parent->address) + ":" + String(c->parent->address) + "," +
                 String(c->state); // manual wrong, says ':' between state
    printf("%s\n", res.c_str());
    udp.beginPacketMulticast(*state.notifyIp, state.notifyPort, ipAddr);
    udp.print(res);
    udp.endPacket();
  }
}

void GCIT::discovery() {
  int i;
  String res;

  if (state.enableTcp && state.enableDiscovery) {
    // always run first time; then throttle based on intervalDiscovery (not random)
    if (state.discoveryTimer == 0) {
      state.discoveryTimer = millis();
    } else if (millis() <= state.discoveryTimer + state.discoveryInterval) {
      return;
    }
    state.discoveryTimer += state.discoveryInterval;

    res = "AMXB<-UUID=GlobalCache_";
    for (i = 0; i < 6; i++) {
      res += String(MAC[i], HEX);
    }

    res += "><-SDKClass=Utility><-Make=GlobalCache><-Model=" + state.model + ">" + "<-Revision=" + state.version +
           "><-Pkg_Level=GCPK001><-Config-URL=http://";
    res += ipString(ipAddr);
    res += "><-PCB_PN=025-0026-06><-Status=Ready>\r";

    printf("discovery beacon:\n%s\n", res.c_str());
    udp.beginPacketMulticast(*state.beaconIp, state.beaconPort, ipAddr);
    udp.print(res);
    udp.endPacket();
    if (broadcastHandler != NULL) {
      (*broadcastHandler)(res);
    }
  }
}

// other possible responses:
//  busyIR (conflict on incoming vs current)

String GCIT::doCommand(String req, WiFiClient *client) {
  int i, len, hexval;
  String res;
  char mod, colon, conn, comma, value;
  String rest, err;
  bool noResponse = (client == NULL);
  Module *module;
  Connector *connector;

  // 0. check if \r or \r\n at end - real one gives err16 if not
  if (req.charAt(req.length() - 1) == '\n') {
    req = req.substring(0, req.length() - 1);
  }
  if (req.charAt(req.length() - 1) != '\r') {
    res = ERR_16;
  } else {

    // 1. remove trailing
    // just remove \r so sendserial doesn't lose chars
    //req.trim();
    req = req.substring(0, req.length() - 1);

    // 2. parse
    if (req.equals("getdevices")) {
      res = "";
      for (i = 0; i < MAX_MODULES; i++) {
        module = getModule(i);
        if (module) {
          res += "device," + String(module->address) + "," + module->descriptor + "\r";
        }
      }
      res += "endlistdevices";

    } else if (req.equals("getversion")) {
      res = state.version;
    } else if (req.substring(0, 11).equals("getversion,")) {
      mod = req.charAt(11);
      err = validateMod(mod, rest);
      if (err.equals("")) {
        res = "version," + String(mod) + "," + state.version;
      } else {
        res = err;
      }

      // gc manual claims this shouldn't have a CR at end!
    } else if (req.equals("get_NET,0:1")) {
      res = "NET,0:1," + String(state.locked ? "LOCKED" : "UNLOCKED") + "," + String(state.dhcp ? "DHCP" : "STATIC") + "," + ipString(ipAddr) + "," +
            ipString(WiFi.subnetMask()) + "," + ipString(WiFi.gatewayIP());

    } else if (req.substring(0, 7).equals("get_IR,")) {
      mod = req.charAt(7);
      colon = req.charAt(8);
      conn = req.charAt(9);
      rest = req.substring(10);
      err = validate(mod, colon, conn, rest);
      if (err.equals("")) {
        module = getModule((unsigned int)(mod - '0'));
        if (!module) {
          res = ERR_02;
        } else {
          connector = module->getConnector((unsigned int)(conn - '0'));
          if (!connector) {
            res = ERR_03;
          } else {
            res = "IR," + String(mod) + ':' + String(conn) + "," + connector->descriptor;
          }
        }
      } else {
        res = err;
      }

    } else if (req.substring(0, 7).equals("sendir,")) {
      mod = req.charAt(7);
      colon = req.charAt(8);
      conn = req.charAt(9);
      comma = req.charAt(10);
      rest = req.substring(11);
      err = validateModCon(mod, colon, conn, comma, ',');
      if (err.equals("")) {
        module = getModule((unsigned int)(mod - '0'));
        if (!module) {
          res = ERR_02;
        } else {
          connector = module->getConnector((unsigned int)(conn - '0'));
          if (!connector) {
            res = ERR_03;
          } else {
            if ((connector->type != ConnectorType::ir) && (connector->type != ConnectorType::irBlaster)) {
              res = ERR_13; // i guess
            } else {
              noResponse = true; // ir will respond
              res = doIR(req, client);
              printf("doIr(%s)\n", req.c_str());
              printf("%s\n", fixup(res).c_str());
            }
          }
        }
      } else {
        res = err;
      }
      // this is actually complicated for some reason; a second connection can come in and do a stopir and
      //  BOTH should be notified (the originating connection then doesn't get a completeir);  apparently trying
      //  to avoid conflicts if a new client wants to start using it? could've just rejected the new request
      //} else if (req.substring(0, 7).equals("stopir,")) {
      //  res = req.substring(0, 10);

    } else if (req.substring(0, 9).equals("getstate,")) {
      mod = req.charAt(9);
      colon = req.charAt(10);
      conn = req.charAt(11);
      rest = req.substring(12);
      err = validate(mod, colon, conn, rest);
      if (err.equals("")) {
        module = getModule((unsigned int)(mod - '0'));
        if (!module) {
          res = ERR_02;
        } else {
          connector = module->getConnector((unsigned int)(conn - '0'));
          if (!connector) {
            res = ERR_03;
          } else {
            if ((connector->type == ConnectorType::sensor) || (connector->type == ConnectorType::sensorNotify)) {
              res = "state," + String(mod) + ':' + String(conn) + "," + String(((ConnectorSensor *)connector->control)->getState());
            } else if ((connector->type == ConnectorType::relay)) {
              res = "state," + String(mod) + ':' + String(conn) + "," + String(((ConnectorRelay *)connector->control)->getState());
            } else {
              res = ERR_18;
            }
          }
        }
      } else {
        res = err;
      }

    } else if (req.substring(0, 9).equals("setstate,")) {
      mod = req.charAt(9);
      colon = req.charAt(10);
      conn = req.charAt(11);
      comma = req.charAt(12);
      value = req.charAt(13);
      rest = req.substring(14);
      err = validate(mod, colon, conn, rest);
      if (err.equals("")) {
        module = getModule((unsigned int)(mod - '0'));
        if (!module) {
          res = ERR_02;
        } else {
          connector = module->getConnector((unsigned int)(conn - '0'));
          if (!connector) {
            res = ERR_03;
          } else {
            if (connector->type == ConnectorType::relay) {
              if ((value >= '0') && (value <= '1')) {
                ((ConnectorRelay *)connector->control)->setState(value - '0');
                // gc manual missing comma
                res = "state," + String(mod) + ':' + String(conn) + "," + String(((ConnectorRelay *)connector->control)->getState());
              } else {
                res = ERR_22; // i guess
              }
            } else {
              res = ERR_18;
            }
          }
        }
      } else {
        res = err;
      }

      //} else if (req.substring(0, 7).equals("set_IR,")) {
      //}

      //} else if (req.substring(0, 11).equals("set_SERIAL,")) {
      //}

      //} else if (req.substring(0, 11).equals("get_SERIAL,")) {
      //}

    /* Not defined by GC; enable sending serial through a command */
    // if req is already uri decoded, %00 won't work (string terminates early);
    //  use sendserialx instead
    } else if (req.substring(0, 11).equals("sendserial,")) {
      mod = req.charAt(11);
      colon = req.charAt(12);
      conn = req.charAt(13);
      comma = req.charAt(14);
      rest = req.substring(15);
      err = validateModCon(mod, colon, conn, comma, ',');
      if (err.equals("")) {
        module = getModule((unsigned int)(mod - '0'));
        if (!module) {
          res = ERR_02;
        } else {
          connector = module->getConnector((unsigned int)(conn - '0'));
          if (!connector) {
            res = ERR_03;
          } else {
            if (connector->type != ConnectorType::serial) {
              res = ERR_13; // i guess
            } else {
              res = "Sent " + String(rest.length()) + " bytes";
              ((ConnectorSerial *)(connector->control))->send(rest);
              printf("sendserial: %s\n", urlencode(rest).c_str());
            }
          }
        }
      } else {
        res = err;
      }

    /* Not defined by GC; enable sending serial through a command */
    // data is string of hex (eg. c001babe)
    } else if (req.substring(0, 12).equals("sendserialx,")) {
      mod = req.charAt(12);
      colon = req.charAt(13);
      conn = req.charAt(14);
      comma = req.charAt(15);
      rest = req.substring(16);
      err = validateModCon(mod, colon, conn, comma, ',');
      if (err.equals("")) {
        module = getModule((unsigned int)(mod - '0'));
        if (!module) {
          res = ERR_02;
        } else {
          connector = module->getConnector((unsigned int)(conn - '0'));
          if (!connector) {
            res = ERR_03;
          } else {
            if (connector->type != ConnectorType::serial) {
              res = ERR_13; // i guess
            } else {
              char buffer[rest.length() / 2];
              i = len = 0;
              while (i < rest.length()) {
                value = toupper(rest.charAt(i++));
                if (isxdigit(value)) {
                  hexval = (value >= 'A') ? (value - 'A' + 10) : (value - '0');
                  hexval = hexval << 4;
                } else {
                  break;
                }
                value = toupper(rest.charAt(i++));
                if (isxdigit(value)) {
                  hexval += (value >= 'A') ? (value - 'A' + 10) : (value - '0');
                  buffer[len++] = hexval;
                } else {
                  break;
                }
              }
              res = "Sent " + String(len) + " bytes";
              ((ConnectorSerial *)(connector->control))->send(buffer, len);
              printf("sendserialx: %s\n", rest.c_str());
            }
          }
        }
      } else {
        res = err;
      }
    }

    else {
      // res = "unknowncommand,ERR_01";     // not what real one does
      res = "ERR_0:0,001";
    }
  }

  if (!noResponse) {
    res += "\r";
    tcpResponse(res, client);
  }
  return res;
}

void GCIT::tcpResponse(String res, WiFiClient *client) {
  client->print(res);
  printf("TCP response: %s\n", fixup(res).c_str());
  client->flush();
}

String GCIT::doCommand(String req) {
  // make it look like tcp
  String res = doCommand(req + "\r", NULL);
  return res;
}

// handle possible multiple commands per receive; split by cr/crlf
void GCIT::tcpCommands(String cmds, WiFiClient *client) {
  int p;
  String req;

  while (cmds.length() > 0) {
    p = cmds.indexOf("\r");
    if (p == -1) { // bad format
      p = cmds.length();
    } else {
      if (cmds.charAt(p + 1) == '\n') {
        p = p + 2;
      } else {
        p = p + 1;
      }
    }
    req = cmds.substring(0, p);
    cmds = cmds.substring(p);
    doCommand(req, client);
  }
}

void GCIT::handleClient() {
  int i;
  bool ok = false;
  String req;

  if (state.enableTcp) {

    if (server->hasClient()) {
      for (i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (serverClientActive[i] && !serverClient[i].connected()) {
          printf("TCP: stopping old client %i\n", i);
          serverClient[i].stop();
          printf("TCP: replacing with new client\n");
          serverClient[i] = server->available();
          ok = true;
          break;
        } else if (!serverClientActive[i]) {
          printf("TCP: inserting client %i\n", i);
          serverClient[i] = server->available();
          serverClientActive[i] = true;
          ok = true;
          break;
        }
      }
      // no free spot - deny or close by lru?
      if (!ok) {
        printf("TCP: ignoring new client!\n");
        WiFiClient serverClient = server->available();
        serverClient.stop();
      }
    }

    for (i = 0; i < MAX_TCP_CLIENTS; i++) {
      if (serverClientActive[i] && serverClient[i].connected()) {
        if (serverClient[i].available()) {
          req = serverClient[i].readString();
          printf("TCP request(%i): %s\n", i, fixup(req).c_str());
          tcpCommands(req, &serverClient[i]);
        }
      } else if (serverClientActive[i] && !serverClient[i].connected()) {
        printf("TCP: deleting disconnected client %i\n", i);
        serverClient[i].stop();
        serverClientActive[i] = false;
      }
    }
  }
}
