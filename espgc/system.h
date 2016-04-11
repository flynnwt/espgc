#pragma once

#include <ESP.h>
#include <FS.h>

#include <utilities.h>
#include <GCIT.h>
#include <flasher.h>

#include "config.h"
#include "status.h"

extern String resetFile;                 // reset info
extern String statusFile;                // boot info
extern String lockFile;                  // lock info
extern String configFile;                // config info
extern unsigned long checkForReset;      // ms

extern Config* config;
extern GCIT *gc;
extern Status *status;

void espInfo();
void fsInfo();
void restart();
void reset();

void resetCheck();
void bootCheck();
void bootStart();
void bootComplete();
bool lockCheck();
void resetCheck();
