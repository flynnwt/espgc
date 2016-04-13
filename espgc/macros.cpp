#include "macros.h"

Macros::Macros() {
  root =  "/macros/";
}

Macros::Macros(String r) {
  root = r;
}

unsigned long Macros::delay() {
  return cmdDelay;
}

void Macros::delay(unsigned long ms) {
  cmdDelay = ms;
}

File Macros::openMacro(String name) {
  return openMacro(name, "r");
}

File Macros::openMacro(String name, String mode) {
  String macroFile;

  if (name.charAt(0) == '/') {
    name = name.substring(1);
  }
  name += ext;
  macroFile = root + name;
  File f = SPIFFS.open(macroFile, mode.c_str());
  if (!f) {
    Serial.printf("Could not open %s.\n", macroFile.c_str());
  }
  return f;
}

String Macros::add(String name,  String cmd) {
  String res = "";
  File f = openMacro(name, "a+");
  
  if (f) {
    f.printf("%s\n", cmd.c_str());
    f.seek(0, SeekMode::SeekSet);
    res = f.readString();
    f.close();
  }
  return res;
}

String Macros::get(String name) {
  String res = "";
  File f = openMacro(name);

  if (f) {
    res = f.readString();
    f.close();
  }
  return res;
}

int Macros::remove(String name) {
  String macroFile;

  if (name.charAt(0) == '/') {
    name = name.substring(1);
  }
  macroFile = root + name + ext;
  return SPIFFS.remove(macroFile);
}
