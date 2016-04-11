#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <Time.h>

#include <utilities.h>
#include <GCIT.h>

#define MAX_CONNECTORS (MAX_MODULES * MODULE_MAX_CONNECTORS)

#define MODULE_PARMS 1
#define CONNECTOR_PARMS 5
#define NONE -1

extern GCIT* gc;

class Config {

public:
  //bool valid;    
  //time_t created;
  //unsigned int timezone;  
  //bool dst; 

  void(*loader)(String text);
  void(*saver)();

  bool locked;
  String version;
  String platform;
  String mac;
  time_t updated;

  String deviceName;
  String tagline;
  String hostname;

  int wirelessMode;
  String ssid;
  String passphrase;
  int dhcp;
  IPAddress staticIp;
  IPAddress gatewayIp;
  IPAddress subnetMask;

  int enableDiscovery;
  int enableTcp;
  int skipped;

  String lastBoot;

  int modules[MAX_MODULES][MODULE_PARMS];                                            // type, statusPin, resetPin(module 0)
  int connectors[MAX_MODULES*MODULE_MAX_CONNECTORS][CONNECTOR_PARMS];                // type, module, address, fcnPin, statusPin

  Config();
  Config(String mac);
  void clear();
  void set(String property, String value);

  String toString();
  JSON toJSON();

  String timeString(time_t t);

};
