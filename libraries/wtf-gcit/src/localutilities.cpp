#include "localutilities.h"

// general

String fixup(String s) {
  s.replace("\r", "<CR>");
  s.replace("\n", "<LF>");
  return s;
}

// these could have strict itach mode

String validateMod(char mod, String rest) {

  if ((mod < '0') || (mod > '9') || !rest.equals("")) {
    return ERR_02;
  } else {
    return "";
  }
}

String validateModCon(char mod, char colon, char conn) {

  if ((mod < '0') || (mod > '9') || (colon != ':')) {
    return ERR_02;
  } else if ((conn < '0') || (conn > '9')) {
    return ERR_03;
  } else {
    return "";
  }
}

String validateModCon(char mod, char colon, char conn, char next, char nextCompare) {

  if ((mod < '0') || (mod > '9') || (colon != ':')) {
    return ERR_02;
  } else if ((conn < '0') || (conn > '9') || (next != nextCompare)) {
    return ERR_03;
  } else {
    return "";
  }
}

String validate(char mod, char colon, char conn, String rest) {

  if ((mod < '0') || (mod > '9') || (colon != ':')) {
    return ERR_02;
  } else if ((conn < '0') || (conn > '9') || !rest.equals("")) {
    return ERR_03;
  } else {
    return "";
  }
}
