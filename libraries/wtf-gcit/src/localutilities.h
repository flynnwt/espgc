#ifndef LOCALUTILITIES_H
#define LOCALUTILITIES_H

#include <WString.h>
#include <WiFiClient.h>
#include "GCITCodes.h"

String fixup(String s);
String validateMod(char mod, String rest);
String validateModCon(char mod, char colon, char conn);
String validateModCon(char mod, char colon, char conn, char next, char nextCompare);
String validate(char mod, char colon, char conn, String rest);

#endif