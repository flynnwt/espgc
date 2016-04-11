#include "wifi.h"

// scan networks
// https://github.com/sandeepmistry/esp8266-Arduino/blob/master/esp8266com/esp8266/libraries/ESP8266WiFi/examples/WiFiScan/WiFiScan.ino
// https://github.com/esp8266/Arduino/blob/d108a6ec30193a8e44eb9c72efc7a55853b54f09/hardware/esp8266com/esp8266/libraries/ESP8266WiFi/src/ESP8266WiFiMulti.cpp

String encryptionType(uint8_t e) {
  String res;

  switch (e) {
  case ENC_TYPE_WEP: res = "WEP "; break;
  case ENC_TYPE_TKIP: res = "TKIP"; break;
  case ENC_TYPE_CCMP: res = "CCMP"; break;
  case ENC_TYPE_NONE: res = "NONE"; break;
  case ENC_TYPE_AUTO: res = "AUTO"; break;
  default: res = "????"; break;
  }
  return res;
}

// synchronous version
void wifiScan() {
  int i, n;
  String ssid;
  uint8_t enc;
  int32_t RSSI;
  uint8_t* BSSID;
  int32_t channel;
  bool isHidden;

  n = WiFi.scanNetworks(false);
  yield();
  for (i = 0; i < n; i++) {
    WiFi.getNetworkInfo(i, ssid, enc, RSSI, BSSID, channel, isHidden);
    Serial.printf(" %d: [%d][%02X:%02X:%02X:%02X:%02X:%02X] %s %s (%d)\n", i, channel, BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5],
      encryptionType(enc).c_str(), ssid.c_str(), RSSI);
  }
  WiFi.scanDelete();    
}

String wifiScanJSON() {
  int i, n;
  String ssid;
  uint8_t enc;
  int32_t RSSI;
  uint8_t* BSSID;
  int32_t channel;
  bool isHidden;
  String res = "{\"networks\":[";

  n = WiFi.scanNetworks(false);
  yield();
  for (i = 0; i < n; i++) {
    WiFi.getNetworkInfo(i, ssid, enc, RSSI, BSSID, channel, isHidden);
    if (i > 0) {
      res += ",";
    }
    res += "\n{";
    res += "\"channel\":" + String(channel) + ",";
    res += "\"bssid\":\"" + zeroPadHex(BSSID[0]) + ":" + zeroPadHex(BSSID[1]) + ":" + zeroPadHex(BSSID[2]) + ":" +
      zeroPadHex(BSSID[3]) + ":" + zeroPadHex(BSSID[4]) + ":" + zeroPadHex(BSSID[5]) + "\",";
    res += "\"encryption\":\"" + encryptionType(enc) + "\",";
    res += "\"ssid\":\"" + ssid + "\",";
    res += "\"rssi\":" + String(RSSI);
    res += "}";
  }
  WiFi.scanDelete();    
  res += "\n]}";
  return res;
}

// asynchronous version
void wifiScanStart() {
  WiFi.scanNetworks(true);
}

bool wifiScanProcess() {
  int8_t n;
  int i;
  String ssid;
  uint8_t enc;
  int32_t RSSI;
  uint8_t* BSSID;
  int32_t channel;
  bool isHidden;

  n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    return false;
  } else {
    for (i = 0; i < n; i++) {
      WiFi.getNetworkInfo(i, ssid, enc, RSSI, BSSID, channel, isHidden);
      Serial.printf(" %d: [%d][%02X:%02X:%02X:%02X:%02X:%02X] %s %s (%d)\n", i, channel, BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5],
        encryptionType(enc).c_str(), ssid.c_str(), RSSI);
    }
    WiFi.scanDelete();    // clean up ram
    return true;
  }
}

