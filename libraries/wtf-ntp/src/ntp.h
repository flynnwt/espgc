#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include <Time.h>

#include <utilities.h>

void ntpBegin();
void ntpBegin(time_t interval);
String ntpTimestamp();
String ntpTimestamp(unsigned long t);
