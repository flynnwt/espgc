// to enable debug in ESP8266WebServer.cpp, Parsing.cpp, etc. (uses DEBUG_OUTPUT), set in them: #define DEBUG 
// #define DEBUGV(...) ets_printf(__VA_ARGS__) in library debug.h
// Serial.setDebugOutput(true) 

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>
#include <ESP.h>
#include <FS.h>

#include <Time.h>
#include <IRremoteESP8266.h>

#include <utilities.h>
#include <GCIT.h>
#include <ntp.h>   
#include <flasher.h>

#include "system.h"
#include "config.h"
#include "status.h"
#include "router.h"
#include "wifi.h"      

#define MAX_CONNECTORS (MAX_MODULES * MODULE_MAX_CONNECTORS)

// **************************************************************************************
// Hard configuration

#define VERSION "867.5309.01"
String resetFile = "/reset.txt";      // reset info
String statusFile = "/status.txt";    // boot info
String lockFile = "/lock.txt";        // lock info
String configFile = "/config.txt";    // user setup

// now could be saved in configFile too...
unsigned int httpPort = 80;
unsigned long waitForWiFi = 20000;    // ms
unsigned long waitForNTP = 20000;     // ms, 0=skip
unsigned long checkForReset = 5000;   // ms
unsigned long updateTime = 60;        // s
IPAddress apIP(192, 168, 0, 1);       // for AP mode

// **************************************************************************************
// Other globals

EspClass* esp = new EspClass();
Config* config;
GCIT *gc;
ESP8266WebServer *server;
bool skipConfig = false;              // if previous boot was bad
int enableTcp, enableDiscovery;       // deferred at start 
Status *status;

// **************************************************************************************
// Utilities

String getMAC() {
  uint8_t MAC[6];
  int i;
  String res = "";

  WiFi.macAddress(MAC);
  for (i = 0; i < 6; i++) {
    res += zeroPadHex(MAC[i]);
  }
  res.toUpperCase();
  return res;
}

String startTimestamp;
void showtime() {
  static time_t prevDisplay = 0;
  unsigned int latency = 0;
  static unsigned long loops = 0;
  //static time_t startTime = millis(); // undefined reference to `__cxa_guard_acquire
  static time_t startTime = 0;
  // static String startTimestamp;  // undefined reference to `__cxa_guard_acquire

  if (loops == 0) {
    startTimestamp = ntpTimestamp();
    startTime = millis();
  }
  loops++;

  if (timeStatus() != timeNotSet) {
    if (now() >= prevDisplay + updateTime) {
      latency = (millis() - startTime) * 1000L / loops;
      prevDisplay = now();
      // hmmm...i dont think they leaking anymore
      Serial.printf("\n*** %s free:%i lat:%ius (boot: %s)\n\n", ntpTimestamp().c_str(), esp->getFreeHeap(), latency, startTimestamp.c_str()); 
    }
  }
}

void initStatus() {

  /*
  // using defined flash rates and controlling all together
  Flasher::Config rgbStatus[3] = {      // status (aithinker devboard rgb)
    { 15, 500, 500 },
    { 12, 500, 1500 },
    { 13, 500, 2500 },
  };
  status = new Status(rgbStatus, 3);
  status->set(1);
  status->start();
  */

  /*
  //using defined states and controlling with state changes
  unsigned int rgb[3] = { 15,12,13 };     // status (aithinker devboard rgb)
  status = new Status(rgb, 3);
  unsigned long statusError[3][2] = { { 1, 0 }, { 0, 0 }, { 0, 0 } };             // solid red
  unsigned long statusReset[3][2] = { { 100, 100 }, { 0, 0 }, { 0, 0 } };         // zippy red 
  unsigned long statusBoot[3][2] = { { 100, 900 }, { 0, 0 }, { 0, 0 } };          // blippy red
  unsigned long statusAP[3][2] = { { 100, 100 }, { 100, 100 }, { 100, 100 } };    // zippy white
  unsigned long statusWiFi[3][2] = { { 250, 1750 }, { 0,0 }, { 250, 1750 } };     // pokey purple
  status->defineState(Status::error, statusError, 3);
  status->defineState(Status::reset, statusReset, 3);
  status->defineState(Status::boot, statusBoot, 3);
  status->defineState(Status::ap, statusAP, 3);
  status->defineState(Status::wifi, statusWiFi, 3);
  */

  // none
  status = new Status();
}

// **************************************************************************************
// Configuration file handling
// back to the darkages...use a non-json text file for now...
// well already there with cpp anyway i guess

// # = comment
// blank = skip
// parm=val\n

// some amount of validation attempted
// should probably keep protected item(s) like passphrase (not viewable if locked) in a separate file

String getConfigFile() {
  String res = "";
  File f = SPIFFS.open(configFile, "r");

  if (!f) {
    Serial.printf("Could not open %s.\n", configFile.c_str());
  } else {
    res = f.readString();
    f.close();
  }
  return res;
}

// deferTcp: don't set tcp/discovery until ready later on (startInfra).  setting in config will
//  cause them to change in gc (which isn't really necessary if want to wait until next reboot).
// no guarantees what havoc is wreaked if changed from web (until reboot); but same is true for
//  other settings that won't take affect until reboot, like wireless stuff.  
// would be smart if it verified config also (settings/checksum)
void loadConfig(String text, bool deferTcp) {
  int p1, p2 = 0, p3;
  String line;

  config->clear();

  while ((p1 = text.indexOf("\n", p2)) != -1) {
    line = text.substring(p2, p1);
    line.replace("\r", "");  // in case manually created in windoze
    if (line.length() > 0) {
      p3 = line.indexOf("=");
      if (p3 > 0) {  // should be 1 or more
        if (deferTcp && (line.substring(0, p3).equals("tcp"))) {
          enableTcp = line.substring(p3 + 1).toInt();
        } else if (deferTcp && (line.substring(0, p3).equals("discovery"))) {
          enableDiscovery = line.substring(p3 + 1).toInt();
        } else {
          config->set(line.substring(0, p3), line.substring(p3 + 1));
        }
      }
    }
    p2 = p1 + 1;
  }
  // check if anything left (no ending nl)
  if (p2 != text.length()) {
    line = text.substring(p2);
    line.replace("\r", "");  // in case manually created in windoze
    p3 = line.indexOf("=");
    if (p3 > 0) {  // should be 1 or more
      if (line.substring(0, p3).equals("tcp")) {
        enableTcp = line.substring(p3 + 1).toInt();
      } else if (line.substring(0, p3).equals("discovery")) {
        enableDiscovery = line.substring(p3 + 1).toInt();
      } else {
        config->set(line.substring(0, p3), line.substring(p3 + 1));
      }
    }
  }

  Serial.println("\nLoaded configuration:");
  Serial.print(config->toString());
  Serial.println();
  Serial.print(config->toJSON().stringify());
  Serial.println();
}

void saveConfig() {
  saveConfig("", "");
}

void saveConfig(String text) {
  saveConfig(text, "");
}

void saveConfig(String text, String prepend) {
  File f;

  Serial.printf("Saving configuration %s...\n", configFile.c_str());
  SPIFFS.remove(configFile);
  f = SPIFFS.open(configFile, "w");
  if (!f) {
    Serial.printf("Could not create %s.\n", configFile.c_str());
  } else {
    if (text.equals("")) {
      text = config->toString();
    }
    if (!prepend.equals("")) {
      f.println(prepend);
    }
    f.print(text);
    f.close();
  }
}

void saveReloadConfig(String text) {
  String configText;

  saveConfig(text);
  loadConfig(text, false);
}

void saveDefaultConfig() {
  // minimal config defaults...
  config->platform = "";      // could do stuff like check for allowable pin usages
  config->ssid = "ESPGC_" + config->mac;
  config->hostname = config->ssid;
  config->deviceName = config->ssid;
  config->tagline = "...we have assumed control...";
  // ...plus default to (wifi, 3 ir), with ir pin 5, sensor_notify pin 4, ir_blaster pin 5
  config->modules[0][0] = (int)ModuleType::wifi;
  config->modules[1][0] = (int)ModuleType::ir;
  config->connectors[0][0] = (int)ConnectorType::ir;
  config->connectors[0][1] = 1;
  config->connectors[0][2] = 1;
  config->connectors[0][3] = 5;
  config->connectors[1][0] = (int)ConnectorType::sensorNotify;
  config->connectors[1][1] = 1;
  config->connectors[1][2] = 2;
  config->connectors[1][3] = 4;
  config->connectors[2][0] = (int)ConnectorType::irBlaster;
  config->connectors[2][1] = 1;
  config->connectors[2][2] = 3;
  config->connectors[2][3] = 5; // go ahead, try pin 6!!!!!!
  saveConfig("", "# Default Configuration");
}

// **************************************************************************************
// Wireless Setup

int startAP(String ssid) {
  Serial.println("Starting as AP...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid.c_str(), NULL);
  Serial.printf(" SSID: %s\n", ssid.c_str());
  Serial.printf(" IP:   %s\n", ipString(apIP).c_str());
  return 1;
}

// it looks like esp remembers previous ssid/passphrase - you can restart it w/o specifying them and it connects!!!
int startInfra(String ssid, String passphrase, int dhcp, IPAddress staticIp, IPAddress gatewayIp, IPAddress subnetMask) {
  IPAddress ipAddr;
  time_t ms;
  bool OK = true;

  if (!dhcp) {
    WiFi.config(staticIp, gatewayIp, subnetMask);
  }
  WiFi.begin(ssid.c_str(), passphrase.c_str());

  ms = millis();
  Serial.print("Connecting to network...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if (millis() - ms > waitForWiFi) {
      OK = false;
      break;
    }
    resetCheck();
    status->process();
    delay(50);
  }
  Serial.println();

  if (OK) {
    ipAddr = WiFi.localIP();

    Serial.printf(" SSID: %s\n", ssid.c_str());
    Serial.printf(" IP:   %s\n", ipString(ipAddr).c_str());

    // allow skipping tcp/discovery
    config->enableTcp = enableTcp;
    config->enableDiscovery = enableDiscovery;
    gc->enableTcp(config->enableTcp);
    if (config->enableTcp) {
      if (config->enableDiscovery) {
        Serial.println("Discovery beacon active on " + String(gc->settings().beaconPort) + ".");
      }
      Serial.println("TCP server started on " + String(gc->settings().tcpPort) + ".");
    }

    if (waitForNTP > 0) {
      Serial.print("Getting time...");
      ntpBegin(3600);
      OK = true;
      ms = millis();
      while (timeStatus() == timeNotSet) {
        Serial.print(".");
        if (millis() - ms > waitForNTP) {
          OK = false;
          break;
        }
        resetCheck();
        status->process();
        delay(50);
      }
      Serial.println();
      if (OK) {
        Serial.println(ntpTimestamp());
      }
    }

    // still return OK if ntp skipped/failed
    gc->setIpAddr(ipAddr);

    return 1;

  } else {
    return 0;
  }

}

// **************************************************************************************
// Setup 

void setup(void) {
  int i, j, rc;
  String MACString, configText;

  Serial.begin(115200);
  Serial.setDebugOutput(false); // WIFI libraries, etc.

  initStatus();
  status->start();
  status->set(Status::error);

  MACString = getMAC();

  gc = new GCIT();
  config = new Config();
  config->version = VERSION;
  config->mac = MACString;

  SPIFFS.begin();
  espInfo();
  fsInfo();

  status->set(Status::reset); // only meaningful for reset if synchronous
  resetCheck();
  status->set(Status::boot);    

  // Persistent Configuration 
  Serial.println("\n\n*** Configuration\n");
  bootCheck();

  if (!SPIFFS.exists(configFile)) {
    Serial.printf("Configuration file %s does not exist.  Creating...\n", configFile.c_str());
    saveDefaultConfig();
  }

  Serial.printf("\nCurrent configuration:\n");
  configText = getConfigFile();
  Serial.print(configText);
  loadConfig(configText, true);

  Serial.print("\nChecking lock: ");
  if (lockCheck()) {
    Serial.println("locked");
  } else {
    Serial.println("unlocked");
  }
  Serial.println();

  // let stuff get to console before crash....
  delay(1000);

  Serial.println("\n\n*** Hardware Configuration\n");

  Module* m[MAX_MODULES];
  for (i = 0; i < MAX_MODULES; i++) {
    m[i] = NULL;
  }

  if (!skipConfig) {
    for (i = 0; i < MAX_MODULES; i++) {
      if (config->modules[i][0] != NONE) {
        Serial.printf("Adding module %i, type=%i\n", i, config->modules[i][0]);
        m[i] = gc->addModule(i, (ModuleType)config->modules[i][0]);
      }
    }

    for (i = 0; i < MAX_CONNECTORS; i++) {
      if (config->connectors[i][0] != NONE) {
        if (m[config->connectors[i][1]]) {
          Serial.printf("Adding connector %i:%i, type=%i, fPin=%i, sPin=%i\n", config->connectors[i][1], config->connectors[i][2], config->connectors[i][0], config->connectors[i][3], config->connectors[i][4]);
          rc = m[config->connectors[i][1]]->addConnector((ConnectorType)config->connectors[i][0], config->connectors[i][2], config->connectors[i][3], config->connectors[i][4]);
          if (!rc) {
            Serial.printf("FAILED!\n");
          }
        } else {
          Serial.printf("NOT adding connector %i:%i, type=%i, fPin=%i, sPin=%i (bad module)\n", config->connectors[i][1], config->connectors[i][2], config->connectors[i][0], config->connectors[i][3], config->connectors[i][4]);
        }
      }
    }

    Serial.println();
  }

  // show tree
  Serial.println("Device Tree:");
  for (i = 0; i < MAX_MODULES; i++) {
    if (m[i]) {
      Serial.printf("Module %i, %s\n", i, m[i]->descriptor.c_str());
      for (j = 0; j < MODULE_MAX_CONNECTORS; j++) {
        if (m[i]->connectors[j]) {
          Serial.printf("  Connector %i:%i, %s, fPin=%i, sPin=%i\n", i, m[i]->connectors[j]->address, m[i]->connectors[j]->descriptor.c_str(),
            m[i]->connectors[j]->control->fcnPin, m[i]->connectors[j]->control->statusPin);
        }
      }
    }
  }

  // Initialization

  Serial.println("\n\n*** Initialization\n");

  Serial.printf("MAC: %s\n", MACString.c_str());
  Serial.printf("Hostname: %s\n", config->hostname.c_str());
  WiFi.hostname(config->hostname);

  if (config->wirelessMode == 0) {

    startAP(config->ssid);
    status->set(Status::ap);

  } else {

    if (startInfra(config->ssid, config->passphrase, config->dhcp, config->staticIp, config->gatewayIp, config->subnetMask)) {
      status->set(Status::wifi);
    } else {
      Serial.printf("\nCould not connect to network!\n");
      startAP(config->ssid);
      status->set(Status::ap);
    }

  }

  bootComplete();

  server = new ESP8266WebServer(httpPort);
  new Router(server, gc);
  server->begin();
  Serial.println("HTTP server started.");

  // just playin...
  Serial.println("\n\n*** Network Scan\n");
  wifiScan();
  //Serial.println(wifiScanJSON());

  Serial.println("\n\n*** Setup Complete...Thunderbirds are GO!\n\n");

}

// **************************************************************************************
// Loop

void loop(void) {
  status->process();
  resetCheck();
  server->handleClient();
  gc->process();
  showtime();
}