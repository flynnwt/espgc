#pragma once

#include <Arduino.h>
#include <FS.h>

class Macros {
  String root;
  String ext = "txt";
  unsigned long cmdDelay = 100;

  File openMacro(String name);
  File openMacro(String name, String mode);

public:
  Macros();
  Macros(String root);

  unsigned long delay();
  void delay(unsigned long ms);
  String add(String name, String cmd);
  String get(String name);  // array return?
  int remove(String name);

};
