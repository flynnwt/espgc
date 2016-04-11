#pragma once

#include <ESP8266WiFi.h>
#include "utilities.h"

int startAP(String ssid);


String encryptionType(uint8_t e);
void wifiScan(void);
String wifiScanJSON(void);
void wifiScanStart(void);
bool wifiScanProcess(void);