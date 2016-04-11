#ifndef WTF_UTILITIES_H
#define WTF_UTILITIES_H

#include <time.h>

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#undef min // NEED THIS AFTER UPDATING ESP8266 ARDUINO TO 2.1
#undef max // NEED THIS AFTER UPDATING ESP8266 ARDUINO TO 2.1
#include <vector>

extern "C" {
#include "user_interface.h"
}
// somewhere later in the code...
// system_restart();

String ipString(IPAddress ipAddr);
IPAddress ipFromString(String ipAddr);
unsigned int getInt(String s);
String trim(String s, char c);
std::vector<String> splitText(String text, char sep, int index = 0);

String urldecode(String url);
String urlencode(String str);

String zeroPad(int i);
String zeroPadHex(int i);

// simple single-level JSON
class JSON {
  String object;
  bool empty;
  void _add(String prop, String val);
  void _addString(String prop, String val);

public:
  String separator = "\n";
  JSON();
  void add(String prop, String val);
  void add(String prop, int val);
  void add(String prop, unsigned int val);
  void add(String prop, long val);
  void add(String prop, unsigned long val);
  void add(String prop, double val);
  void addJSONString(String prop, String val);
  String stringify(void);
};

#endif