#ifndef GCIT_h
#define GCIT_h

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>
#include <IRremoteESP8266.h>

#include <utilities.h>
#include <logger.h>

#include "GCITCodes.h"
#include "localutilities.h"
#include "ir.h"
#include "module.h"
#include "contact.h"
#include "serial.h"
#include "connector.h"

#define MAX_TCP_CLIENTS 3
#define MAX_MODULES 8
#define MODULE_MAX_CONNECTORS 8

#define UNDEFINED_MOD -1
#define UNDEFINED_CONN -1
#define UNDEFINED_FPIN 128  // allow negative values for relay out negative-active 
#define UNDEFINED_SPIN -1

enum class ModuleType { wifi, ethernet, ir, serial, relay };
enum class ConnectorType {
  ir,
  irBlaster,
  sensor,
  sensorNotify,
  ledLighting,
  serial,
  relay
};

class GCIT;
class Module;
class Connector;

// specific connector state and control
class ConnectorControl {
public:
  GCIT *gc;
  Connector *parent;
  int fcnPin = UNDEFINED_FPIN;
  int statusPin = UNDEFINED_SPIN;

  ConnectorControl() {}
  ConnectorControl(Connector *c);
  ConnectorControl(Connector *c, int fPin);
  ConnectorControl(Connector *c, int fPin, int sPin);

  void setStatus(unsigned int v);
  void flashStatus();
  // this should specify all possible connector functions so they don't have to be
  //  cast when called?  anything not overridden return error
};

class ConnectorNetPower : public ConnectorControl {};

class ConnectorIr : public ConnectorControl {
  IRsend *irSend = NULL;

public:
  ConnectorIr(Connector *parent);
  ConnectorIr(Connector *parent, int fPin);
  ConnectorIr(Connector *parent, int fPin, int sPin);
  void sendIRRaw(unsigned int Hz, unsigned int len, unsigned int irSignal[]);
  void sendIRRaw(unsigned int Hz, unsigned int len, unsigned int irSignal[],
                 unsigned int offset);
};

class ConnectorSensor : public ConnectorControl {
  unsigned int state = 0;

public:
  ConnectorSensor(Connector *c, int fPin, int sPin);
  unsigned int getState();
};

class ConnectorSensorNotify : public ConnectorControl {
  unsigned long timer;
  unsigned long interval;

public:
  unsigned int state;
  ConnectorSensorNotify(Connector *c, int fPin, int sPin);
  void setInterval(unsigned long ms);
  void update();
};

class ConnectorLedLighting : public ConnectorControl {
  unsigned int intensity1;
  unsigned int intensity2;

public:
  ConnectorLedLighting(Connector *c, int fPin, int sPin) {
    ConnectorControl(c, fPin, sPin);
  }
};

class ConnectorRelay : public ConnectorControl {
  unsigned int state;

public:
  ConnectorRelay(Connector *c, int fPin, int sPin);
  unsigned int getState();
  void setState(unsigned int s);
};

// this really doesn't fit since it uses 2 function pins and potentially 2 status pins...
// possibly would add Connector::Connector() with array of fPin, SPin
// but since must use Serial, and it currently must use pins 1/3 or 13/15, just let it be
//  set up by user; then could use fPin, SPin as rxSPin, txSPin 
class ConnectorSerial : public ConnectorControl {

public: 
  enum class FlowControl {hardware, none};
  enum class Parity {none, odd, even};

private:  
  // bogus; this is just duplicating the main version so it should be a separate class
  //  instantiated in both places
  bool enableTcp;
  WiFiClient serverClient[MAX_TCP_CLIENTS];
  bool serverClientActive[MAX_TCP_CLIENTS];
  WiFiServer *server;
   
  String fixup(char *buffer, unsigned int len);

public:
  unsigned int baudRate;
  FlowControl flowControl;
  Parity parity;
  unsigned int recvBufferSize;
  char *recvBuffer;
  unsigned int recvBufferLen;
  bool recvBufferOFlow;

  ConnectorSerial(Connector *c); 
  ConnectorSerial(Connector *c, int rxSPin, int txSPin);
  unsigned int set(unsigned int baudRate); 
  unsigned int set(unsigned int baudRate, FlowControl flowControl, Parity parity); 
  void reset();
  void setBufferSize(unsigned int size);
  String getParams();
  void startTcpServer();
  void send(String s);
  void send(char buffer[], unsigned int len);
  //void (*receiveCB)(char buffer[], unsigned int len) = NULL;
  void process();
};

class Connector {
public:
  Module *parent;
  int address;
  int error = 0;
  ConnectorType type;
  ConnectorControl *control;
  String descriptor;

  Connector(Module *m, unsigned int a, ConnectorType t);
  Connector(Module *m, unsigned int a, ConnectorType t, int fPin);
  Connector(Module *m, unsigned int a, ConnectorType t, int fPin, int sPin);
};

class Module {
public:
  GCIT *parent;
  ModuleType type;
  int address;
  Connector *connectors[MODULE_MAX_CONNECTORS];
  int numConnectors;
  String descriptor;

  Module(GCIT *p, unsigned int a, ModuleType t);

  int addConnector(ConnectorType t);
  int addConnector(ConnectorType t, int fPin);
  int addConnector(ConnectorType t, int fPin, int sPin);
  int addConnector(ConnectorType t, int address, int fPin, int sPin);
  Connector *getConnector(int address);
};

// miscellaneous settings and status
class Settings {

public:
  String version;
  String model;
  IPAddress *beaconIp;
  int beaconPort;
  unsigned long discoveryInterval;
  unsigned long discoveryTimer;
  IPAddress *notifyIp;
  int notifyPort;
  bool locked;
  bool dhcp;
  bool enableDiscovery;
  bool enableTcp;
  int tcpPort;
  int serialTcpPort;

  Settings() {
    version = "710-1001-05";
    model = "";
    beaconIp = new IPAddress(239, 255, 250, 250);
    beaconPort = 9131;
    discoveryInterval = 60000;
    discoveryTimer = 0;
    notifyIp = new IPAddress(239, 255, 250, 250);
    notifyPort = 9132;
    locked = false;
    dhcp = true;
    enableDiscovery = true;
    enableTcp = true;
    tcpPort = 4998;
    serialTcpPort = 4999;
  }
};

// Could pass index into server array instead of actual client pointer.  would allow extra info to be
//  kept for each connection (quiesce timeout, etc.); also, allows responsePending which is needed
//  for busyir and stopir
class ServerClient {
  WiFiClient client;
  unsigned int id;
  unsigned long timer;
  unsigned long interval;
  bool pendingIr; // busy with IR (most likely); set when command parsed, clear
                  // in responder or
  //  stopir handler (also have to check new ir requests for a pending one)
};

class GCIT {

  WiFiUDP udp;
  uint8_t MAC[6];
  IPAddress ipAddr; 
  WiFiServer *server;
  WiFiClient serverClient[MAX_TCP_CLIENTS];
  bool serverClientActive[MAX_TCP_CLIENTS];

  Module *modules[MAX_MODULES];
  unsigned int numModules;
  ConnectorSerial *serialConnector = NULL;  // set if a serial conn exists to enable process()
  
  Settings state;

  String doIR(String req, WiFiClient *client);
  void tcpResponse(String resp, WiFiClient *client);
  void tcpCommands(String resp, WiFiClient *client);
  String doCommand(String cmd, WiFiClient *client);

  void init();
  void sendIRRaw(unsigned int kHz, unsigned int len, unsigned int irSignal[]);
  void discovery();
  void checkSensors();
  void handleClient();

public:
  void printf(const char* format, ...);
  HardwareSerial *serial;
  Log *logger;

  void (*broadcastHandler)(String res) = NULL;
  GCIT();
  GCIT(ModuleType t);

  static ModuleType getModuleType(String t);
  static ConnectorType getConnectorType(String t);
  static String getModuleType(ModuleType t);
  static String getConnectorType(ConnectorType t);
  Module *addModule(ModuleType t);
  Module *addModule(int address, ModuleType t);
  Module *getModule(unsigned int m);
  // connector numbering starts at 1!!!
  Connector *getConnector(unsigned int m, unsigned int c);
  
  void setIpAddr(IPAddress ip);
  void setTcp(bool enable);
  void startTcpServer();
  void startTcpServer(int port);
  void stopTcpServer();
  
  Settings* settings();
  void lock(bool enable);
  void enableDiscovery(bool enable);
  void enableTcp(bool enable);
  void enableSerial(ConnectorSerial *c);
  
  void process();

  void sensorNotify(ConnectorSensorNotify *);
  static String compressIr(String pairs);
  static String compressIr(String pairs, unsigned int fudge);
  static String decompressIr(String pairs);
  String doCommand(String cmd);
  
};

#endif
