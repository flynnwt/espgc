#include "utilities.h"

String ipString(IPAddress ipAddr) {
  return String(ipAddr[0]) + "." + String(ipAddr[1]) + "." + String(ipAddr[2]) + "." +
         String(ipAddr[3]);
}

IPAddress ipFromString(String ipAddr) {
  unsigned int i, p;
  uint8_t n[4];
  char c;

  p = i = n[0] = 0;
  while ((i < 4) && (p < ipAddr.length())) {
    c = ipAddr.charAt(p);
    if (isDigit(c)) {
      n[i] = n[i] * 10 + (c - '0');
      p++;
    } else if (c == '.') {
      i++;
      n[i] = 0;
      p++;
    } else {
      break;
    }
  }
  return IPAddress(n[0], n[1], n[2], n[3]);
}

unsigned int getInt(String s) {
  unsigned int res = 0;
  int i;
  for (i = 0; i < s.length(); i++) {
    if ((s.charAt(i) >= '0') && (s.charAt(i) <= '9')) {
      res = 10 * res + (s.charAt(i) - '0');
    } else {
      break;
    }
  }
  return res;
}

String trim(String s, char c) {
  int i;
  for (i = 0; i < s.length(); i++) {
    if (s.charAt(i) == c) {
      return s.substring(i + 1);
    }
  }
  return "";
}

String zeroPad(int i) {
  String res;
  if (i < 10) {
    res = "0" + String(i);
  } else {
    res = String(i);
  }
  return res;
}

String zeroPadHex(int i) {
  String res = String(i, HEX);
  if (res.length() < 2) {
    res = "0" + res;
  }
  return res;
}

std::vector<String> splitText(String text, char sep, int index) {
  unsigned int i;
  String token = "";
  std::vector<String> tokens;

  for (i = index; i < text.length(); i++) {
    if (text.charAt(i) == sep) {
      tokens.push_back(token);
      token = "";
    } else {
      token += char(text.charAt(i));
    }
  }
  tokens.push_back(token);
  return tokens;
}

unsigned char x2int(char c) {
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

String int2x(char c) {
  String res = "";
  uint8_t nibble;

  nibble = (c & 0xF0) >> 4;
  if (nibble > 9) {
    res += (char)(nibble - 10 + 'A');
  } else {
    res += (char)(nibble + '0');
  }

  nibble = (c & 0xF);
  if (nibble > 9) {
    res += (char)(nibble - 10 + 'A');
  } else {
    res += (char)(nibble + '0');
  }

  return res;
}

String urldecode(String url) {
  int i = 0;
  char c;
  String res = "";

  while (i < url.length()) {
    c = url.charAt(i);
    if (c == '+')
      c = ' ';
    if (c == '%') {
      i++;
      c = url.charAt(i);
      i++;
      c = (x2int(c) << 4) | x2int(url.charAt(i));
    }
    res += c;
    i++;
  }
  return res;
}

String urlencode(String str) {
  char c;
  String res = "";
  int i = 0;

  while (i < str.length()) {
    c = str.charAt(i);
    if (c == ' ' || isalnum(c)) {
      if (c == ' ') {
        c = '+';
      }
      res += c;
      i++;
      continue;
    }
    res += '%';
    res += int2x(c);
    i++;
  }
  return res;
}

// Simple JSON Stuff
JSON::JSON() {
  object = "";
  empty = true;
}

void JSON::_add(String prop, String val) {
  if (!empty) {
    object += "," + separator;
  } else {
    empty = false;
  }
  object += "\"" + prop + "\":" + val + "";
}

void JSON::_addString(String prop, String val) {
  if (!empty) {
    object += "," + separator;
  } else {
    empty = false;
  }
  // take care of illegal chars
  val.replace("\\", "\\\\");
  val.replace("\"", "\\\"");
  val.replace("\b", "\\b");
  val.replace("\f", "\\f");
  val.replace("\n", "\\n");
  val.replace("\r", "\\r");
  val.replace("\t", "\\t");
  object += "\"" + prop + "\":\"" + val + "\"";
}

void JSON::add(String prop, int val) { _add(prop, String(val)); }

void JSON::add(String prop, unsigned int val) { _add(prop, String(val)); }

void JSON::add(String prop, long val) { _add(prop, String(val)); }

void JSON::add(String prop, unsigned long val) { _add(prop, String(val)); }

void JSON::add(String prop, double val) { _add(prop, String(val)); }

void JSON::add(String prop, String val) { _addString(prop, val); }

void JSON::addJSONString(String prop, String val) { _add(prop, val); }

String JSON::stringify() { return "{" + separator + object + separator + "}"; }
