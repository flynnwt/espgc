#include "config.h"

Config::Config() {
  lastBoot = "";
  clear();
}

void Config::clear() {
  int i, j;

  locked = false;
  //version = "";
  //mac = "";
  platform = "";
  updated = 0;
  deviceName = "";
  tagline = "";
  hostname = "ESPGC";
  wirelessMode = 0;
  ssid = "";
  passphrase = "";
  dhcp = 0;
  staticIp = IPAddress(0, 0, 0, 0);
  gatewayIp = IPAddress(0, 0, 0, 0);
  subnetMask = IPAddress(0, 0, 0, 0);
  enableDiscovery = 0;
  enableTcp = 0;
  //must be cleared explicitly skipped = 0;

  for (i = 0; i < MAX_MODULES; i++) {
    for (j = 0; j < MODULE_PARMS; j++) {
      modules[i][j] = NONE;
    }
  }
  for (i = 0; i < MAX_CONNECTORS; i++) {
    for (j = 0; j < CONNECTOR_PARMS; j++) {
      connectors[i][j] = NONE;
    }
  }
}

// text file: module=num,type             and connector=num,type,mod,addr,fpin,spin
// json:      module_num:"type,fpin,spin" and connector_num: "type,mod,addr,fpin,spin"

void Config::set(String property, String value) {
  unsigned int p1, p2, modNum, connNum, connAddr, fcnPin, statusPin;
  String token, hi, lo;
  ModuleType modType;
  ConnectorType connType;

  property.toLowerCase();
  if (property.equals("version")) {
    //RO version = value;
  } else if (property.equals("platform")) {
    platform = value;
  } else if (property.equals("mac")) {
    //RO mac = value;
  } else if (property.equals("updated")) {
    updated = atol(value.c_str());
  } else if (property.equals("locked")) {
    //RO locked = value;
  } else if (property.equals("devicename")) {
    deviceName = value;
  } else if (property.equals("tagline")) {
    tagline = value;
  } else if (property.equals("hostname")) {
    hostname = value;
  } else if (property.equals("wirelessmode")) {
    wirelessMode = value.toInt();
  } else if (property.equals("ssid")) {
    ssid = value;
  } else if (property.equals("passphrase")) {
    passphrase = value;
  } else if (property.equals("dhcp")) {
    dhcp = value.toInt();
  } else if (property.equals("staticip")) {
    staticIp = ipFromString(value);
  } else if (property.equals("gatewayip")) {
    gatewayIp = ipFromString(value);
  } else if (property.equals("subnetmask")) {
    subnetMask = ipFromString(value);
  } else if (property.equals("discovery")) {
    enableDiscovery = value.toInt();
    gc->enableDiscovery(enableDiscovery);
  } else if (property.equals("tcp")) {
    enableTcp = value.toInt();
    gc->enableTcp(enableTcp);
  } else if (property.equals("skipped")) {
    //must be set explicitly skipped = value.toInt();
  } else if (property.equals("module")) {
    p1 = value.indexOf(",");
    if (p1 != -1) {
      modNum = value.substring(0, p1).toInt();
      p2 = p1;
      p1 = value.indexOf(",", p2 + 1);
      if (modNum < MAX_MODULES) {
        if (p1 == -1) {
          modType = GCIT::getModuleType(value.substring(p2 + 1));
          if ((int)modType != -1) {
            modules[modNum][0] = (int)modType;
          }
        } else {
          /* not using
          modType = GCIT::getModuleType(value.substring(p2 + 1, p1));
          if ((int)modType != -1) {
            modules[modNum][0] = (int)modType;
            p2 = p1;
            p1 = value.indexOf(",", p2 + 1);
            if (p1 == -1) {
              fcnPin = value.substring(p2 + 1).toInt();
              modules[modNum][1] = fcnPin;
            } else {
              fcnPin = value.substring(p2 + 1, p1).toInt();
              modules[modNum][1] = fcnPin;
              statusPin = value.substring(p1 + 1).toInt();
              modules[modNum][2] = statusPin;
            }
          }
          */
        }
      }
    }
  } else if (property.equals("connector")) {
    p1 = value.indexOf(",");
    if (p1 != -1) {
      connNum = value.substring(0, p1).toInt();
      p2 = p1;
      p1 = value.indexOf(",", p2 + 1);
      if (connNum < MAX_CONNECTORS) {
        if (p1 != -1) {
          connType = GCIT::getConnectorType(value.substring(p2 + 1, p1));
          p2 = p1;
          p1 = value.indexOf(",", p2 + 1);
          if (p1 != -1) {
            modNum = value.substring(p2 + 1, p1).toInt();
            p2 = p1;
            p1 = value.indexOf(",", p2 + 1);
            if (p1 != -1) {
              connAddr = value.substring(p2 + 1, p1).toInt();
              if ((int)connType != -1) {
                p2 = p1;
                p1 = value.indexOf(",", p2 + 1);
                connectors[connNum][0] = (int)connType;
                connectors[connNum][1] = modNum;
                connectors[connNum][2] = connAddr;
                if (p1 == -1) {
                  fcnPin = value.substring(p2 + 1).toInt();
                  connectors[connNum][3] = fcnPin;
                } else {
                  fcnPin = value.substring(p2 + 1, p1).toInt();
                  connectors[connNum][3] = fcnPin;
                  statusPin = value.substring(p1 + 1).toInt();
                  connectors[connNum][4] = statusPin;
                }
              }
            }
          }
        }
      }
    }

  }
}

String Config::toString() {
  int i;
  String res = "", type;

  res += "version=" + version + "\n";
  res += "platform=" + platform + "\n";
  res += "mac=" + mac + "\n";
  res += "updated=" + String(updated) + "\n";
  res += "locked=" + String(locked) + "\n";
  res += "devicename=" + deviceName + "\n";
  res += "tagline=" + tagline + "\n";
  res += "hostname=" + hostname + "\n";
  res += "wirelessMode=" + String(wirelessMode) + "\n";
  res += "ssid=" + ssid + "\n";
  if (locked) {
    res += "passphrase=(DEVICE LOCKED)\n";
  } else {
    res += "passphrase=" + passphrase + "\n";
  }
  res += "dhcp=" + String(dhcp) + "\n";
  res += "staticIp=" + ipString(staticIp) + "\n";
  res += "gatewayIp=" + ipString(gatewayIp) + "\n";
  res += "subnetMask=" + ipString(subnetMask) + "\n";
  res += "discovery=" + String(enableDiscovery) + "\n";
  res += "tcp=" + String(enableTcp) + "\n";
  //only show in json res += "skipped=" + String(skipped) + "\n";

  for (i = 0; i < MAX_MODULES; i++) {
    if (modules[i][0] != NONE) {
      res += "module=" + String(i) + "," + GCIT::getModuleType((ModuleType)modules[i][0]) + "\n";
    }
  }

  for (i = 0; i < MAX_CONNECTORS; i++) {
    if (connectors[i][0] != NONE) {
      res += "connector=" + String(i) + "," + GCIT::getConnectorType((ConnectorType)connectors[i][0]) + "," + String(connectors[i][1]) + "," + String(connectors[i][2]) + "," + String(connectors[i][3]) + "," + String(connectors[i][4]) + "\n";
    }
  }

  return res;

}

// chintzy flat json for now
JSON Config::toJSON() {
  int i;
  JSON data;

  data.add("version", version);
  data.add("platform", platform);
  data.add("mac", mac);
  data.add("updated", updated);
  data.add("locked", locked);
  data.add("deviceName", deviceName);
  data.add("tagline", tagline);
  data.add("hostname", hostname);
  data.add("wirelessMode", wirelessMode);
  data.add("ssid", ssid);
  if (locked) {
    data.add("passphrase", "(DEVICE LOCKED)");
  } else {
    data.add("passphrase", passphrase);
  }
  data.add("dhcp", dhcp);
  data.add("staticIp", ipString(staticIp));
  data.add("gatewayIp", ipString(gatewayIp));
  data.add("subnetMask", ipString(subnetMask));
  data.add("discovery", enableDiscovery);
  data.add("tcp", enableTcp);
  data.add("skipped", skipped);

  for (i = 0; i < MAX_MODULES; i++) {
    if (modules[i][0] != NONE) {
      data.add("module_" + String(i),
        //GCIT::getModuleType((ModuleType)modules[i][0]) + "," + String(modules[i][1]) + "," + String(modules[i][2]));
        GCIT::getModuleType((ModuleType)modules[i][0]));
    }
  }
  for (i = 0; i < MAX_CONNECTORS; i++) {
    if (connectors[i][0] != NONE) {
      data.add("connector_" + String(i), GCIT::getConnectorType((ConnectorType)connectors[i][0]) + "," + String(connectors[i][1]) + "," + String(connectors[i][2]) + "," + String(connectors[i][3]) + "," + String(connectors[i][4]));
    }
  }

  data.add("lastBoot", lastBoot);   // just for web

  return data;
}

String Config::timeString(time_t t) {
  String res;
  res = zeroPad(month(t)) + "-" + zeroPad(day(t)) + "-" + String(year(t)) + " " + zeroPad(hour(t)) + ":" + zeroPad(minute(t)) + ":" + zeroPad(second(t)) + " UTC";
  return res;
}
