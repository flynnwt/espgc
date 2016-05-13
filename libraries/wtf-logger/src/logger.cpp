#include "logger.h"

Log::Log() : HardwareSerial(0) {
  init(0);
}

Log::Log(int uart) : HardwareSerial(uart) {
  init(uart);
}

void Log::init(int uart) {
  int i;

  if (uart == 0) {
    serial = &Serial;
  } else if (uart == 1) {
    serial = &Serial1;
  } else {
    serial = &Serial; // or null and check in prints
  }
  _level = level::debug;
  tcpEnable = false;
  tcpPort = 23;
  for (i = 0; i < MAX_TCP_CLIENTS; i++) {
    serverClientActive[i] = false;
  }
}

void Log::begin(unsigned long baud) {
  serial->begin(baud, SERIAL_8N1, SERIAL_FULL);
}

void Log::begin(unsigned long baud, SerialConfig config) {
  serial->begin(baud, config, SERIAL_FULL);
}

void Log::begin(unsigned long baud, SerialConfig config, SerialMode mode) {
  serial->begin(baud, config, mode);
}

Log::level Log::logLevel() {
  return _level;
}

void Log::logLevel(level l) {
  _level = l;
}

String Log::logLevelName() {
  return levelName[(int)_level];
}

String Log::fixup(String s) {
  s.replace("\r", "<CR>");
  s.replace("\n", "<LF>");
  return s;
}

// *** Print Stuff ***
// name lookup stops in first scope found, so you can't override just some
//  of the same-named functions!  darrr.....cpp

size_t Log::printf(const char *format, ...) {
  size_t len = 0;
  va_list arg;
  char temp[1460];

  va_start(arg, format);
  len = ets_vsnprintf(temp, 1460, format, arg);
  va_end(arg);

  tcpSend(temp);
  // HardwareSerial::print(temp);  // wdt hang
  serial->print(temp);

  return len;
}

size_t Log::printf(level l, const char *format, ...) {
  size_t len = 0;
  va_list arg;

  if (l >= _level) {
    va_start(arg, format);
    char temp[1460];
    len = ets_vsnprintf(temp, 1460, format, arg);
    va_end(arg);

    tcpSend(temp);
    serial->print(temp);
  }

  return len;
}

// these allow writing a callback that can take variable printf args and send them here;
//  e.g. a library can optionally have a printf callback that it uses, which could be set up to use
//  Log or Serial by library user
size_t Log::printf(const char *format, va_list arg) {
  size_t len = 0;
  char temp[1460];

  len = ets_vsnprintf(temp, 1460, format, arg);

  tcpSend(temp);
  serial->print(temp);

  return len;
}

size_t Log::printf(level l, const char *format, va_list arg) {
  size_t len = 0;
  char temp[1460];

  if (l >= _level) {
    len = ets_vsnprintf(temp, 1460, format, arg);

    tcpSend(temp);
    serial->print(temp);
  }

  return len;
}

size_t Log::println() {
  tcpSend("\n");
  return serial->println();
}

size_t Log::println(const String &s) {
  return println(level::all, s);
}

size_t Log::println(level l, const String &s) {
  if (l >= _level) {
    tcpSend(s + "\n");
    return serial->println(s);
  } else {
    return 0;
  }
}

size_t Log::println(const char s[]) {
  return println(level::all, s);
}

size_t Log::println(level l, const char s[]) {
  if (l >= _level) {
    tcpSend(s, true);
    return serial->println(s);
  } else {
    return 0;
  }
}

size_t Log::print(const String &s) {
  return print(level::all, s);
}

size_t Log::print(level l, const String &s) {
  if (l >= _level) {
    tcpSend(s);
    return serial->print(s);
  } else {
    return 0;
  }
}

size_t Log::print(const char s[]) {
  return print(level::all, s);
}

size_t Log::print(level l, const char s[]) {
  if (l >= _level) {
    tcpSend(s);
    return serial->print(s);
  } else {
    return 0;
  }
}

// *** TCP Stuff ***

void Log::startTcp() {
  startTcp(tcpPort);
}

void Log::startTcp(unsigned int port) {
  tcpPort = port;
  tcpStart(port);
}

void Log::process() {
  tcpProcess();
}

void Log::tcpStart(int port) {
  tcpEnable = true;
  server = new WiFiServer(tcpPort);
  server->setNoDelay(true);
  server->begin();
  if (!quiet) {
    printf("DEBUGTCP: started server on port %i\n", tcpPort);
  }
}

void Log::tcpProcess() {
  int i;
  bool ok = false;
  String req;

  if (tcpEnable) {

    if (server->hasClient()) {
      for (i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (serverClientActive[i] && !serverClient[i].connected()) {
          if (!quiet) {
            printf("DEBUGTCP: stopping old client %i\n", i);
          }
          serverClient[i].stop();
          if (!quiet) {
            printf("DEBUGTCP: replacing with new client\n", i);
          }
          serverClient[i] = server->available();
          ok = true;
          break;
        } else if (!serverClientActive[i]) {
          if (!quiet) {
            printf("DEBUGTCP: inserting client %i\n", i);
          }
          serverClient[i] = server->available();
          serverClientActive[i] = true;
          ok = true;
          break;
        }
      }
      // no free spot - deny or close by lru?
      if (!ok) {
        if (!quiet) {
          printf("DEBUGTCP: ignoring new client!\n");
        }
        WiFiClient serverClient = server->available();
        serverClient.stop();
      }
    }

    for (i = 0; i < MAX_TCP_CLIENTS; i++) {
      if (serverClientActive[i] && serverClient[i].connected()) {
        if (serverClient[i].available()) {
          req = serverClient[i].readString();
          tcpReceive(req, i);
        }
      } else if (serverClientActive[i] && !serverClient[i].connected()) {
        if (!quiet) {
          printf("DEBUGTCP: deleting disconnected client %i\n", i);
        }
        serverClient[i].stop();
        serverClientActive[i] = false;
      }
    }
  }
}

void Log::tcpSend(String s) {
  tcpSend(s, false);
}

void Log::tcpSend(String s, bool newline) {
  int i;

  if (tcpEnable) {
    for (i = 0; i < MAX_TCP_CLIENTS; i++) {
      if (serverClientActive[i] && serverClient[i].connected()) {
        if (newline) {
          serverClient[i].println(s);
        } else {
          serverClient[i].print(s);
        }
        serverClient[i].flush();
      }
    }
  }
}

void Log::tcpReceive(String req, unsigned int i) {
  if (!quiet) {
    printf("DEBUGTCP received(%i): '%s'\n", i, fixup(req).c_str());
  }
  if (tcpReceiveCB) {
    (tcpReceiveCB)(req);
  }
}
