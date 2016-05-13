#ifndef LOGGER_h
#define LOGGER_h

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

/*
// global
Log logger;          
Log::level debug = Log::level::debug;
Log::level infoo = Log::level::info; // info in libmain
Log::level warning = Log::level::warning;
Log::level error = Log::level::error;

// setup
logger.begin(115200); 
logger.logLevel(warning);

logger.printf(debug, "Debug message\n");
logger.printf(infoo, "Info message\n");
logger.printf(warning, "Warning message\n");
logger.printf(error, "Error message\n");

logger.tcpReceiveCB = tcpReceived; // void tcpReceived(String req) {}, to handle tcp input

// loop
logger.process();      

// use logger instead of Serial in current code
#define Serial logger 
*/

#define MAX_TCP_CLIENTS 3

class Log : public HardwareSerial {
public:
  // use debug, warning, info, error, none for logger level
  // use debug, warning, info, error, all for print level (all is same as printf w/ no level)
  enum class level {debug, info, warning, error, none, all};

protected:
  const String levelName[6] = { "debug", "info", "warning", "error", "none", "all" };
  HardwareSerial *serial;
  
  WiFiServer *server;
  WiFiClient serverClient[MAX_TCP_CLIENTS];
  bool serverClientActive[MAX_TCP_CLIENTS];
  unsigned int tcpPort;
  bool tcpEnable;
  level _level;

  void init(int uart);
  void tcpStart(int port);
  void tcpSend(String s);
  void tcpSend(String s, bool newline);
  void tcpReceive(String s, unsigned int i);
  void tcpProcess();
  String fixup(String s);

public:
  bool quiet = false;
  void (*tcpReceiveCB)(String req) = NULL;

  Log();
  Log(int uart);

  void begin(unsigned long baud);
  void begin(unsigned long baud, SerialConfig config);
  void begin(unsigned long baud, SerialConfig config, SerialMode mode);
  
  level logLevel();
  void logLevel(level l);
  String logLevelName();

  void startTcp(unsigned int port);
  void startTcp();

  size_t printf(const char *format, ...);
  size_t printf(level l, const char *format, ...);
  size_t printf(const char *format, va_list arg);
  size_t printf(level l, const char *format, va_list arg);
  
  size_t println();
  size_t println(const String &s);
  size_t println(level l, const String &s);
  size_t println(const char[]);
  size_t println(level l, const char[]);

  size_t print(const String &s);
  size_t print(level l, const String &s);
  size_t print(const char[]);
  size_t print(level l, const char[]);
  
  void process();

};

#endif