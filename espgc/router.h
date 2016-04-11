#ifndef ROUTER_H
#define ROUTER_H

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>
#include <IRremoteESP8266.h>
#include <ESP.h>
#include <FS.h>

#include "GCIT.h"
#include "wifi.h"
#include "config.h" 

class Router;

extern Router* router;
extern Config* config;
extern String getConfigFile();
extern void saveConfig();
extern void saveReloadConfig(String text);
extern void restart();
extern bool lockCheck();
extern String lock(String pw);
extern String unlock(String pw);

class Router {
  GCIT* gc;
  EspClass* esp = new EspClass();

  int numMIME = 3;
  String MIMESuffixes[3] = {
    ".html",
    ".css",
    ".js"
  };
  String MIMETypes[3] = {
    "text/html",
    "text/css",
    "application/x-javascript"
  };

  void init(void);

public:
  ESP8266WebServer* server;

  Router(ESP8266WebServer* s, GCIT* g) {
    server = s;
    router = this;    // so cpp will let me do a cb
    gc = g;
    init();
  }
  void handleAPI(void);
  void handleNotFound(void);

};

#endif