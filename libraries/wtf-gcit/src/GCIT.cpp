#include "GCIT.h"

void GCIT::init() {
  int i;

  server = NULL;
  state.enableTcp = false;
  state.tcpPort = 4998;
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
    return (ModuleType)-1;
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
    return (ConnectorType)-1;
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
}

/*
sensor notify broadcast (9132)
*/

void GCIT::sensorNotify(ConnectorSensorNotify *c) {
  if (state.enableTcp) {
    String res = "sensornotify," + String(c->parent->parent->address) + ":" + String(c->parent->address) + "," +
                 String(c->state); // manual wrong, says ':' between state
    Serial.println(res);
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

    Serial.println("discovery beacon:");
    Serial.println(res);
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
  int i;
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
    req.trim();

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
            }
          }
        }
      } else {
        res = err;
      }
      // this is actually complicated for some reason; a second connection can come in and do a stopir and
      //  BOTH should be notified (the originating connection then doesn't get a completeir);  apparently trying
      //  to avoid conflicts if a new client wants to start using it? could've just rejected the new request
    } else if (req.substring(0, 7).equals("stopir,")) {
      res = req.substring(0, 10);

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

    } else {
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
  Serial.print("TCP response: ");
  Serial.println(fixup(res));
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
    /*
    p = cmds.indexOf("\r\n");
    if (p == -1) { // bad format
      p = cmds.length();
    }
    req = cmds.substring(0, p + 2);
    cmds = cmds.substring(p + 2);
    */
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
          Serial.print("TCP: stopping old client ");
          Serial.println(i);
          serverClient[i].stop();
          Serial.print("TCP: replacing with new client ");
          Serial.println(i);
          serverClient[i] = server->available();
          ok = true;
        } else if (!serverClientActive[i]) {
          Serial.print("TCP: inserting client ");
          Serial.println(i);
          serverClient[i] = server->available();
          serverClientActive[i] = true;
          ok = true;
        }
      }
      // no free spot - deny or close by lru?
      if (!ok) {
        Serial.println("TCP: ignoring new client!");
        WiFiClient serverClient = server->available();
        serverClient.stop();
      }
    }

    for (i = 0; i < MAX_TCP_CLIENTS; i++) {
      if (serverClientActive[i] && serverClient[i].connected()) {
        if (serverClient[i].available()) {
          req = serverClient[i].readString();
          Serial.print("TCP request(");
          Serial.print(i);
          Serial.print("): ");
          Serial.println(fixup(req));
          tcpCommands(req, &serverClient[i]);
        }
      } else if (serverClientActive[i] && !serverClient[i].connected()) {
        Serial.print("TCP: deleting disconnected client ");
        Serial.println(i);
        serverClient[i].stop();
        serverClientActive[i] = false;
      }
    }
  }
}
