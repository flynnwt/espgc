#include "system.h"

void espInfo() {
  Serial.println("\n\n*** Hardware Info\n");

  EspClass* e = new EspClass();
  Serial.println("ESP Info: ");
  Serial.print(" VCC: ");
  Serial.printf("%i\n", e->getVcc());
  Serial.print(" Frequency: ");
  Serial.printf("%i\n", e->getCpuFreqMHz());
  Serial.print(" Free Heap: ");
  Serial.printf("%i\n", e->getFreeHeap());
  Serial.print(" SDK: ");
  Serial.printf("%s\n", e->getSdkVersion());
  Serial.print(" Boot Version: ");
  Serial.printf("%i\n", e->getBootVersion());
  Serial.print(" Boot Mode: ");
  Serial.printf("%i\n", e->getBootMode());

  Serial.print(" Flash: ");
  Serial.printf("%i,", e->getFlashChipId(), HEX);
  Serial.printf("%i,", e->getFlashChipSize());  // this is from config, not flash chip id decode!!!
  Serial.printf("%i,", e->getFlashChipSpeed());
  Serial.printf("%i\n", e->getFlashChipMode());
}

void fsInfo() {
  Dir dir;
  File f;

  Serial.println("\n\n*** File System\n");

  Serial.println("Dir: /");
  dir = SPIFFS.openDir("/");
  while (dir.next()) {
    f = dir.openFile("r");
    Serial.printf("%s: %iB\n", dir.fileName().c_str(), f.size());
  }
  Serial.println();

}

void restart() {
  File f;

  if (SPIFFS.exists(statusFile)) {
    SPIFFS.remove(statusFile);
  }
  if (f = SPIFFS.open(statusFile, "w")) {
    f.print("OK:RESTART");
    f.close();
  } else {
    Serial.printf("Could not write %s.\n", statusFile.c_str());
  }
  Serial.println("Restarting...");
  //delay(10000);
  system_restart();
}

void reset() {
  File f;

  Serial.println("Resetting lock, status, and configuration.");
  SPIFFS.remove(lockFile);
  SPIFFS.remove(statusFile);
  SPIFFS.remove(configFile);

  if (f = SPIFFS.open(statusFile, "w")) {
    f.print("OK:RESET");
    f.close();
  } else {
    Serial.printf("Could not write %s.\n", statusFile.c_str());
  }
  Serial.println("Restarting...");
  //delay(10000);
  system_restart();
}

// **************************************************************************************
// Boot handling
// check if last boot was OK (did not reset before completing wireless config);
//   in case a bad pin is used in hardware config.  if configuration is skipped,
//   it won't be the next boot (assuming wireless is OK).

void bootCheck() {
  File f;
  String statusText;

  Serial.print("Checking last boot: ");
  config->skipped = false;
  if (SPIFFS.exists(statusFile)) {
    if (f = SPIFFS.open(statusFile, "r")) {
      statusText = f.readString();
      Serial.printf("status=%s\n", statusText.c_str());
      config->lastBoot = statusText;
      if (!statusText.substring(0, 2).equals("OK")) {
        config->skipped = true;
      }
      f.close();
    } else {
      Serial.printf("could not read %s.\n", statusFile.c_str());
      config->lastBoot = "could not read status file.";
      config->skipped = true;
    }
    SPIFFS.remove(statusFile);
  } else {
    Serial.printf("%s does not exist.\n", statusFile.c_str());
    config->lastBoot = "Status file missing.";
    config->skipped = true;  // or false?
  }
  if (config->skipped) {
    Serial.println("Last boot was not OK.  Skipping hardware configuration.");
  }
  bootStart();
}

void bootStart() {
  File f;

  if (SPIFFS.exists(statusFile)) {
    SPIFFS.remove(statusFile);
  }
  if (f = SPIFFS.open(statusFile, "w")) {
    f.print("FAILED");
    f.close();
  } else {
    Serial.printf("Could not write %s.\n", statusFile.c_str());
  }
}

void bootComplete() {
  File f;

  if (SPIFFS.exists(statusFile)) {
    SPIFFS.remove(statusFile);
  }
  if (f = SPIFFS.open(statusFile, "w")) {
    f.print("OK");
    f.close();
  } else {
    Serial.printf("Could not write %s.\n", statusFile.c_str());
  }
  Serial.println("Boot complete.");
}

// **************************************************************************************
// Lock Handling
// Allow some functions to be blocked if device locked.  Should reset device after changing state.

bool lockCheck() {
  File f;

  config->locked = SPIFFS.exists(lockFile);
  gc->lock(config->locked);
  return config->locked;

}

String lock(String pw) {
  File f;
  String res;

  // allows blank pw
  if (lockCheck()) {
    res = "Can't lock device, already locked";
  } else {
    if (f = SPIFFS.open(lockFile, "w")) {
      f.print(pw);
      f.close();
      if (lockCheck()) {
        Serial.printf("pw:\n%s\n", pw.c_str());
        res = "Locked device";
      } else {
        res = "Lock failed";
      }
    } else {
      res = "Can't lock device, could not write lock file";
    }
  }
  Serial.println(res);
  return res;
}

String unlock(String pw) {
  File f;
  String text;
  String res;

  if (lockCheck()) {
    if (f = SPIFFS.open(lockFile, "r")) {
      text = f.readString();
      f.close();
      if (text.equals(pw)) {
        SPIFFS.remove(lockFile);
        if (lockCheck()) {
          res = "Unlock failed";
        } else {
          res = "Unlocked device";
        }
      } else {
        res = "Can't unlock device, password mismatch";
      }
    } else {
      res = "Can't unlock device, could not open lock file";
    }
  } else {
    res = "Can't unlock device, already unlocked";
  }
  Serial.println(res);
  return res;
}

// **************************************************************************************
// Reset Handling
// Allow a reset pin to be specified, which is checked during boot.
// Two ways to handle:
//   asynchronous - call in all loops, etc.; it stops checking if not config'd or time expires
//   synchronous  - call once and don't leave unless not config'd or time expires or pin is wrong state
// Must use synchronous if afraid system may crash before a reset can be detected and save the day.
// 
// resetFile:
//   p=s   continue
//   p=s!  wait
// no checking; a bad reset file can crash system

void resetCheck() {
  File f;
  String text;
  unsigned int p;
  static bool waitHere = true;
  static int resetPin = -1, resetState = 0, done = false;
  static time_t startTime = 0;

  if (!done) {
    if ((resetPin == -1) && (startTime == 0)) {
      Serial.print("Checking reset file: ");
      if (SPIFFS.exists(resetFile)) {
        if (f = SPIFFS.open(resetFile, "r")) {
          text = f.readString();
          f.close();
          p = text.indexOf("=");
          resetPin = text.substring(0, p).toInt();
          resetState = text.substring(p + 1).toInt();
          waitHere = text.substring(text.length() - 1).equals("!");
          pinMode(resetPin, INPUT);
          startTime = millis();
          p = digitalRead(resetPin);
          if (p == resetState) {
            Serial.printf("pin:%i state:%i\n", resetPin, resetState);
            // still checking...
            //  async - must do at least on more pass to get reset
            //  sync  - wait here to make final decision
            Serial.print("Synchronous reset - waiting until completion.");
            if (waitHere) {
              while (!done && (millis() - startTime < checkForReset)) {
                Serial.print(".");
                p = digitalRead(resetPin);
                done = (p != resetState);
                status->process();
                delay(50);
              }
              Serial.println();
              if (done) {
                Serial.printf("No reset - pin %i state not %i.\n", resetPin, resetState);
              } else {
                Serial.println("Reset time complete and pin was held.");
                done = true;
                reset();
              }
            }
          } else {
            Serial.printf("No reset - pin %i state not %i.\n", resetPin, resetState);
            done = true;
          }
        } else {
          Serial.printf("could not read %s.\n", resetFile.c_str());
          done = true; // assume it's permanently bad
        }
      } else {
        Serial.printf("%s does not exist.\n", resetFile.c_str());
        done = true;
      }
    } else {
      p = digitalRead(resetPin);
      if (p == resetState) {
        if (millis() - startTime > checkForReset) {
          Serial.println("Reset time complete and pin was held.");
          done = true;
          reset();
        } else {
          // still checking...
        }
      } else {
        Serial.printf("No reset - pin %i state not %i.\n", resetPin, resetState);
        done = true;
      }
    }
  }
}

