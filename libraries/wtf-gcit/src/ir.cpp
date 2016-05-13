#include "ir.h"
/*
(sendir,<connectoraddress>,<ID>,<frequency>,<repeat>,<offset>,...)
sendir,1:1,1,37000,10,1,128,64,16,16,16,49,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,49,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,49,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,49,16,16,16,49,16,49,16,49,16,49,16,16,16,16,16,49,16,16,16,49,16,49,16,49,16,49,16,16,16,49,16,2718
*/

ConnectorIr::ConnectorIr(Connector *c) {
  ConnectorIr(c, UNDEFINED_FPIN, UNDEFINED_SPIN);
}

ConnectorIr::ConnectorIr(Connector *c, int fPin) {
  ConnectorIr(c, fPin, UNDEFINED_SPIN);
}

ConnectorIr::ConnectorIr(Connector *c, int fPin, int sPin) {
  parent = c;
  fcnPin = fPin;
  statusPin = sPin;
  if (fcnPin != UNDEFINED_FPIN) {
    pinMode(fcnPin, OUTPUT);
    irSend = new IRsend(fcnPin);
    if (irSend) {
      irSend->begin();
    }
  }
  if (statusPin != UNDEFINED_SPIN) {
    pinMode(statusPin, OUTPUT);
  }
}

String GCIT::doIR(String req, WiFiClient *client) {
  int i, period;
  String res;
  // char colon, comma;
  char mod, conn;
  String rest, err;
  unsigned int id, freq, repeat, offset, codeLen;
  unsigned int pairs[260 * 2];
  Module *module;
  Connector *connector;

  mod = req.charAt(7);
  // colon = req.charAt(8);
  conn = req.charAt(9);
  // comma = req.charAt(10);
  rest = req.substring(11);

  // done before call here
  // err = validateModCon(mod, colon, conn);
  // if (!err.equals("")) {
  //  res = err;
  //} else {
  module = getModule((unsigned int)(mod - '0'));
  if (!module) {
    res = ERR_02;
  } else {
    connector = module->getConnector((unsigned int)(conn - '0'));
    if (!connector) {
      res = ERR_03;
    } else {
      id = getInt(rest);
      rest = trim(rest, ',');
      if (id > 65535) {
        res = ERR_04;
      } else {
        freq = getInt(rest);
        rest = trim(rest, ',');
        if ((freq < 15000) || (freq > 500000)) {
          res = ERR_05;
        } else {
          period = 1000000.0 * (1.0 / (double)freq); // integer microseconds
          repeat = getInt(rest);
          rest = trim(rest, ',');
          if ((repeat < 1) || (repeat > 50)) {
            res = ERR_06;
          } else {
            offset = getInt(rest);
            rest = trim(rest, ',');
            if ((offset < 1) || (offset > 383)) {
              res = ERR_07;
            } else {
              rest = CompressedIR().decode(rest); // decompress
              for (i = 0; i < 260 * 2; i += 2) {
                if (rest.equals("")) {
                  break;
                }
                pairs[i] = getInt(rest);
                rest = trim(rest, ',');
                // gc spec wrong i assume (says 65635)
                if ((pairs[i] < 1) || (pairs[i] > 65535)) {
                  err = ERR_09;
                }
                pairs[i + 1] = getInt(rest);
                rest = trim(rest, ',');
                if ((pairs[i + 1] < 1) || (pairs[i + 1] > 65535)) {
                  err = ERR_09;
                }
                // convert pair values to microseconds
                pairs[i] = pairs[i] * period;
                pairs[i + 1] = pairs[i + 1] * period;
              }
              if (!rest.equals("")) {
                res = ERR_08;
              } else if (!err.equals("")) {
                res = err;
              } else {
                codeLen = i;
                ((ConnectorIr *)(connector->control))->sendIRRaw(freq, codeLen, pairs);
                delay(10);
                // offset should be odd, and starts at 1 for first pair; so point into pairs
                //  at correct spot and adjust codeLen
                for (i = 1; i < repeat; i++) {
                  ((ConnectorIr *)(connector->control))->sendIRRaw(freq, codeLen - (offset - 1), pairs, offset - 1);
                  delay(10);
                }
                res = "completeir," + String(mod) + ":" + String(conn) + "," + String(id);
              }
            }
          }
        }
      }
    }
  }
  //}

  if (client) {
    res += "\r";
    tcpResponse(res, client);
  }
  return res;
}

// for web/api
String GCIT::compressIr(String pairs) {
  return CompressedIR().encode(pairs);
}

// not really useful unless doing learning and using raw timings (before converting to period counts)
String GCIT::compressIr(String s, unsigned int f) {
  return CompressedIR().encode(s, f);
} 

String GCIT::decompressIr(String pairs) {
  return CompressedIR().decode(pairs);
}

// IR Connectors

void ConnectorIr::sendIRRaw(unsigned int Hz, unsigned int len, unsigned int irSignal[]) {
  sendIRRaw(Hz, len, irSignal, 0);
}

void ConnectorIr::sendIRRaw(unsigned int Hz, unsigned int len, unsigned int irSignal[], unsigned int offset) {
  setStatus(HIGH);
  if (irSend) {
    noInterrupts();
    irSend->sendRaw(&irSignal[offset], len, (unsigned int)(Hz / 1000));
    interrupts();
  }
  setStatus(LOW);
}

// IR Code Compress/Decompress
//
// tokens are:
//  on,off
//  var
// comma separates on|off and off|on, but NOT var|on, off|var, var|var
// iconvert.exe inserts comma for var|on case!  and devices appear to accept both as legal...
//    i.e. only numbers are separated
// string starts with on
// string ends with off or var
//
// fudge used with raw timings (learned) to smooth samples

unsigned int CompressedIR::getInt(String s) {
  unsigned int res = 0;
  int i;
  for (i = p; i < s.length(); i++) {
    if ((s.charAt(i) >= '0') && (s.charAt(i) <= '9')) {
      res = 10 * res + (s.charAt(i) - '0');
    } else {
      break;
    }
  }
  p = i;
  return res;
}

bool CompressedIR::isVar(String s) {
  return (s.charAt(p) >= 'A') && (s.charAt(p) <= 'Z');
}

unsigned int CompressedIR::codePos(char c) {
  return c - 'A';
}

char CompressedIR::codeName(unsigned int n) {
  return n + 'A';
}

char CompressedIR::checkPair(unsigned int on, unsigned int off) {
  int i;

  if (fudge != 0) {
    return checkPairFudged(on, off, fudge);
  }

  for (i = 0; i < numPairs; i++) {
    if ((library[i][0] == on) && (library[i][1] == off)) {
      return codeName(i);
    }
  }
  return 0;
}

// fuzzy equality
// a smart one would first attempt to bucket them, then use an average of the buckets
char CompressedIR::checkPairFudged(unsigned int on, unsigned int off, unsigned int fudge) {
  int i;
  bool matchOn, matchOff;
  unsigned long check0, check1;

  for (i = 0; i < numPairs; i++) {
    check0 = (unsigned long)on * (100 - (unsigned long)fudge) / 100;
    check1 = (unsigned long)on * (100 + (unsigned long)fudge) / 100;
    matchOn = (library[i][0] >= check0) && (library[i][0] <= check1);
    check0 = (unsigned long)off * (unsigned long)(100 - fudge) / 100;
    check1 = (unsigned long)off * (unsigned long)(100 + fudge) / 100;
    matchOff = (library[i][1] >= check0) && (library[i][1] <= check1);
    if (matchOn && matchOff) {
      return codeName(i);
    }
  }
  return 0;
}

char CompressedIR::addPair(unsigned int on, unsigned int off) {

  if (numPairs < librarySize) {
    library[numPairs][0] = on;
    library[numPairs][1] = off;
    numPairs++;
    return codeName(numPairs - 1);
  } else {
    return 0;
  }
}

char CompressedIR::addPairUnique(unsigned int on, unsigned int off) {

  int exists = checkPair(on, off);
  if (exists == 0) {
    return 0;
  } else {
    return addPair(on, off);
  }
}

unsigned int CompressedIR::getPair0(char c) {
  return library[codePos(c)][0];
}

unsigned int CompressedIR::getPair1(char c) {
  return library[codePos(c)][1];
}

// assume well-formed
// returns uncompressed form
String CompressedIR::decode(String s) {
  unsigned int var, on, off;
  String res = "";

  p = 0;
  numPairs = 0;
  while (p < s.length()) {
    if (isVar(s)) {
      var = s.charAt(p);
      res += String(getPair0(var)) + "," + String(getPair1(var)) + ",";
      p++;
    } else {                    // start of new pair; shouldn't be in library!
      if (s.charAt(p) == ',') { // allow optional comma!
        p++;
      }
      on = getInt(s);
      p++; // should be comma
      off = getInt(s);
      // 'off' can be followed by var or ','
      if (!isVar(s)) {
        p++;
      }
      addPair(on, off);
      res += String(on) + "," + String(off) + ",";
    }
  }
  res = res.substring(0, res.length() - 1);
  return res;
}

// assume well-formed
// returns compressed form
String CompressedIR::encode(String s) {
  unsigned int on, off;
  char check;
  bool offLast = false;
  String res = "";

  p = 0;
  numPairs = 0;
  while (p < s.length()) {
    // start new pair (may be in library)
    on = getInt(s);
    p++; // should be comma
    off = getInt(s);
    p++; // comma or end
    check = checkPair(on, off);
    if (check == 0) {
      addPair(on, off);
      if (offLast) {
        res += ",";
      }
      res += String(on) + "," + String(off);
      offLast = true;
    } else {
      res += String(check);
      offLast = false;
    }
  }
  return res;
}

// save/restore
String CompressedIR::encode(String s, unsigned int f) {
  unsigned int old;
  String res;

  old = fudge;
  fudge = f;
  res = encode(s);
  fudge = old;
  return res;
}
