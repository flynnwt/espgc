#include "router.h"

/*
connector commands, GET/POST (same format as TCP)...
http://192.168.0.100/api?cmd=getdevices
http://192.168.0.100/api?cmd=getversion
http://192.168.0.100/api?cmd=get_NET,0:1
http://192.168.0.100/api?cmd=get_IR
http://192.168.0.100/api?cmd=sendir
http://192.168.0.100/api?cmd=getstate
http://192.168.0.100/api?cmd=setstate

connector commands, GET/POST (not defined by GC)...
http://192.168.0.100/api?cmd=sendserial
http://192.168.0.100/api?cmd=sendserialx
http://192.168.0.100/api?cmd=recvserial

macros...
GET  http://192.168.0.100/api/macro
POST  http://192.168.0.100/api/macro
DELETE  http://192.168.0.100/api/macro

other...
GET  http://192.168.0.100/api/config
*POST http://192.168.0.100/api/config
*GET  http://192.168.0.100/api/configfile
*POST http://192.168.0.100/api/configfile
GET  http://192.168.0.100/api/networks
GET  http://192.168.0.100/api/compressir
GET  http://192.168.0.100/api/decompressir

*POST http://192.168.0.100/api/admin/restart
*GET http://192.168.0.100/api/admin/file
*POST http://192.168.0.100/api/admin/file
*DELETE http://192.168.0.100/api/admin/file
POST http://192.168.0.100/api/admin/lock
POST http://192.168.0.100/api/admin/unlock

* locked

possible: dir list, file write

*/

// eventually use json config and get rid of text file
//   1. do chintzy flat version of json; properties module0, module1, ..., connector0, connector1, ...
//   2. don't make general purpose json serializer; just do it with a special purpose function

Router* router;

static void routeHandler(void);
void configGet(void);
void configPost(void);

String decodeMethod(uint8_t m) {
  String method;
  switch (router->server->method()) {
  case HTTP_GET: method = "GET";
    break;
  case HTTP_POST: method = "POST";
    break;
  case HTTP_PUT: method = "PUT";
    break;
  case HTTP_PATCH: method = "PATCH";
    break;
  case HTTP_DELETE: method = "DELETE";
    break;
  case HTTP_OPTIONS: method = "OPTIONS";
    break;
  default:
    method = String(router->server->method()) + "?";
  }
  return method;
}

// like encodeuricomponent(): skip alphabetic, decimal digits, - _ . ! ~ * ' ( ) 
// and ignore 7F+
String decodeURI(String s) {

  s.replace("%09", " ");
  s.replace("%0D%0A", "\n");
  s.replace("%0A", "\n");
  s.replace("+", " ");

  s.replace("%20", " ");
  //s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  //s.replace("%27", "'");
  //s.replace("%28", "(");
  //s.replace("%29", ")");
  //s.replace("%2A", "*");
  s.replace("%2B", "+");
  s.replace("%2C", ",");
  //s.replace("%2D", "-");
  //s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%3A", ":");
  s.replace("%3B", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  //s.replace("%5F", "_");
  s.replace("%60", "`");
  s.replace("%7B", "{");
  s.replace("%7C", "|");
  s.replace("%7D", "}");
  //s.replace("%7E", "~");
  return s;
}

void Router::init() {
  // doesnt work because of implied 'this' for handleRoot() - need to use intermediate wrapper
  //server->on("/", handleRoot);
  //server->on("/", test);
  //server->on("/ir", handleIr);
  server->serveStatic("/", SPIFFS, "/index.html");
  server->serveStatic("/", SPIFFS, "/");
  server->onNotFound(routeHandler);
}

static void routeHandler() {  // all non-static
  int i;

  Serial.printf("%s %s", decodeMethod(router->server->method()).c_str(), router->server->uri().c_str());
  for (i = 0; i < router->server->args(); i++) {
    if (i == 0) {
      Serial.printf("?");
    } else {
      Serial.printf("&");
    }
    Serial.printf("%s=%s", router->server->argName(i).c_str(), router->server->arg(i).c_str());
  }
  Serial.printf("\n");
  if (router->server->uri().substring(0, 4).equals("/api")) {
    router->handleAPI();
  } else {
    router->handleNotFound();
  }
}

/*
void sendJSON(int code, String res) {
  Serial.printf("%i\n%s\n", code, res.c_str());
  router->server->send(code, "application/json", res);
}
*/

void sendJSON(int code, JSON data) {
  Serial.printf("%i\n%s\n", code, data.stringify().c_str());
  router->server->send(code, "application/json", data.stringify());
}

/*
void sendText(int code, String res) {
  Serial.printf("%i\n%s\n", code, res.c_str());
  router->server->send(code, "text/plain", res);
}
*/

void Router::handleNotFound() {
  JSON data;
  String res, method;
  int code;

  method = decodeMethod(server->method());
  if (!method.equals("GET")) {
    data.add("err", "Bad request");
    code = 400;
  } else {
    data.add("err", "File not found");
    code = 404;
  }
  sendJSON(code, data);
}

void configFileGet() {
  JSON data;
  int code;

  if (!config->locked) {
    code = 200;
    data.add("res", getConfigFile());
  } else {
    code = 200; // code = 422;
    data.add("err", "Device is locked");
  }
  sendJSON(code, data);
}

void configFilePost() {
  JSON data;
  String text, res;
  int code;

  if (!config->locked) {
    if (router->server->hasArg("text")) {
      text = decodeURI(router->server->arg("text"));
      saveReloadConfig(text);
      code = 200;
      data.add("res", getConfigFile());
    } else {
      code = 200; // code = 422;
      data.add("err", "Expected 'text' parameter");
    }
  } else {
    code = 200; // code = 422;
    data.add("err", "Device is locked");
  }
  sendJSON(code, data);
}

void configGet() {
  JSON data;
  String res;
  int code = 200;

  data.addJSONString("res", config->toJSON().stringify());
  sendJSON(code, data);
}

// must set updated= explicitly if desired
void configPost() {
  JSON data;
  int i, code;

  if (!config->locked) {
    for (i = 0; i < router->server->args(); i++) {
      Serial.printf("  %s=%s\n", router->server->argName(i).c_str(), urldecode(router->server->arg(i)).c_str());
      config->set(router->server->argName(i), urldecode(router->server->arg(i)));
    }
    saveConfig();
    code = 200;
    data.addJSONString("res", config->toJSON().stringify());
  } else {
    code = 200; // code = 422;
    data.add("err", "Device is locked");
  }
  sendJSON(code, data);
}

// no nested macros or special commands
void macroGet() {
  JSON data;
  int code, p;
  String name, cmds, cmd, res;
  unsigned int cmdDelay;

  if (router->server->hasArg("list")) {
    name = decodeURI(router->server->arg("list"));
    res = macros->get(name);
    data.add("name", name);
    data.add("commands", res);
    code = 200;
  } else if (router->server->hasArg("name")) {
    name = decodeURI(router->server->arg("name"));
    if (router->server->hasArg("delay")) {
      cmdDelay = router->server->arg("delay").toInt();
    } else {
      cmdDelay = macros->delay();
    }
    data.add("name", name);
    cmds = macros->get(name);
    data.add("commands", cmds);
    data.add("delay", cmdDelay);
    while (cmds.length() > 0) {
      p = cmds.indexOf("\n");
      if (p == -1) {
        p = cmds.length();
      }
      cmd = cmds.substring(0, p);
      cmds = cmds.substring(p + 1);
      res += gc->doCommand(cmd) + "\n";
      delay(cmdDelay);
    }
    data.add("res", res);
    code = 200;
  } else {
    code = 200; // code = 422;
    data.add("err", "Expected 'list' or 'exec' parameters");
  }
  sendJSON(code, data);
}

void macroPost() {
  JSON data;
  int code;
  String name, res;

  if (router->server->hasArg("name")) {
    name = decodeURI(router->server->arg("name"));
    if (router->server->hasArg("cmd")) {
      res = macros->add(name, router->server->arg("cmd"));
      data.add("name", name);
      data.add("commands", res);
      code = 200;
    } else {
      code = 200; // code = 422;
      data.add("err", "Expected 'cmd' parameter");
    }
  } else {
    code = 200; // code = 422;
    data.add("err", "Expected 'name' parameter");
  }
  sendJSON(code, data);
}

void macroDelete() {
  JSON data;
  int code;
  bool rc;

  if (router->server->hasArg("name")) {
    rc = macros->remove(decodeURI(router->server->arg("name")));
    if (!rc) {
      data.add("err", "Remove failed");
    } else {
      data.add("res", "OK");
    }
    code = 200;
  } else {
    code = 200; // code = 422;
    data.add("err", "Expected 'list' or 'exec' parameters");
  }
  sendJSON(code, data);
}

void networksGet() {
  JSON data;
  int code = 200;

  data.add("res", wifiScanJSON());
  sendJSON(code, data);
}

void compressIrGet() {
  JSON data;
  int code;
  unsigned int fudge = 0;
  String res, val;

  if (router->server->hasArg("code")) {
    val = decodeURI(router->server->arg("code"));
    if (router->server->hasArg("fudge")) {
      fudge = router->server->arg("fudge").toInt();
      res = GCIT::compressIr(val, fudge);
      data.add("code", res);
      data.add("fudge", fudge);
    } else {
      res = GCIT::compressIr(val);
      data.add("code", res);
    }
    code = 200;
  } else {
    code = 200; // code = 422;
    data.add("err", "Expected 'code' parameter");
  }
  sendJSON(code, data);
}

void decompressIrGet() {
  JSON data;
  int code;
  String res, val;

  if (router->server->hasArg("code")) {
    val = decodeURI(router->server->arg("code"));
    res = GCIT::decompressIr(val);
    code = 200;
    data.add("code", res);
  } else {
    code = 422;
    data.add("err", "Expected 'code' parameter");
  }
  sendJSON(code, data);
}

void fileGet() {
  JSON data;
  String res = "", file;
  int code;
  File f;

  if (!config->locked) {
    if (router->server->hasArg("file")) {
      file = decodeURI(router->server->arg("file"));
      if (file.charAt(0) != '/') {
        file = "/" + file;
      }
      if (SPIFFS.exists(file)) {
        f = SPIFFS.open(file, "r");
        if (f) {
          res = f.readString();
          f.close();
          data.add("name", file);
          data.add("data", res);
          code = 200;
        } else {
          data.add("err", "Open failed");
          code = 200; // code = 422
        }
      } else {
        data.add("err", "File doesn't exist");
        code = 200; // code = 422
      }
    } else {
      data.add("err", "Expected 'file' parameter");
      code = 200; // code = 422
    }
  } else {
    data.add("err", "Device is locked");
    code = 200; // code = 422
  }
  sendJSON(code, data);

}

void filePost() {
  JSON data;
  String res = "", file;
  int code;
  File f;

  if (!config->locked) {
    if (router->server->hasArg("file")) {
      if (router->server->hasArg("data")) {
        file = decodeURI(router->server->arg("file"));
        if (file.charAt(0) != '/') {
          file = "/" + file;
        }
        if (!SPIFFS.exists(file)) {
          f = SPIFFS.open(file, "w");
          if (f) {
            f.print(decodeURI(router->server->arg("data")));
            f.close();
            data.add("name", file);
            data.add("data", decodeURI(router->server->arg("data")));
            code = 200;
          } else {
            data.add("err", "Open failed");
            code = 200; // code = 422
          }
        } else {
          data.add("err", "File exists - not overwriting");
          code = 200; // code = 422
        }
      } else {
        data.add("err", "Expected 'data' parameter");
        code = 200; // code = 422
      }
    } else {
      data.add("err", "Expected 'file' parameter");
      code = 200; // code = 422
    }
  } else {
    data.add("err", "Device is locked");
    code = 200; // code = 422
  }
  sendJSON(code, data);

}

void fileDelete() {
  JSON data;
  String res = "", file;
  int code;

  if (!config->locked) {
    if (router->server->hasArg("file")) {
      file = decodeURI(router->server->arg("file"));
      if (file.charAt(0) != '/') {
        file = "/" + file;
      }
      // must be blind; don't see way to tell if it's a dir or file; appears to only work on file - at least / don't work...
      if (SPIFFS.exists(file)) {
        if (SPIFFS.remove(file)) {
          data.add("res", "OK");
          code = 200;
        } else {
          data.add("err", "Delete failed");
          code = 200; // code = 422
        }
      } else {
        data.add("err", "File doesn't exist");
        code = 200; // code = 422
      }
    } else {
      data.add("err", "Expected 'file' parameter");
      code = 200; // code = 422
    }
  } else {
    data.add("err", "Device is locked");
    code = 200; // code = 422
  }
  sendJSON(code, data);

}

void Router::handleAPI() {
  JSON data;
  String res = "", file;
  int code, method = server->method();

  if (server->uri().equals("/api/config")) {
    if (method == HTTP_POST) {
      configPost();
      return;
    } else if (method == HTTP_GET) {
      configGet();
      return;
    }
  } else if (server->uri().equals("/api/configfile")) {
    if (method == HTTP_POST) {
      configFilePost();
      return;
    } else if (method == HTTP_GET) {
      configFileGet();
      return;
    }
  } else if (server->uri().equals("/api/macro")) {
    if (method == HTTP_POST) {
      macroPost();
      return;
    } else if (method == HTTP_GET) {
      macroGet();
      return;
    } else if (method == HTTP_DELETE) {
      macroDelete();
      return;
    }
  } else if (router->server->uri().equals("/api/networks")) {
    if (server->method() == HTTP_GET) {
      networksGet();
      return;
    }
  } else if (router->server->uri().equals("/api/compressir")) {
    if (server->method() == HTTP_GET) {
      compressIrGet();
      return;
    }
  } else if (router->server->uri().equals("/api/decompressir")) {
    if (server->method() == HTTP_GET) {
      decompressIrGet();
      return;
    }
  } else if (router->server->uri().equals("/api/admin/restart")) {
    if (method == HTTP_POST) {
      if (!config->locked) {
        data.add("res", "OK");
        sendJSON(200, data);
        restart();
      } else {
        data.add("err", "Device is locked");
        code = 200; // code = 422
        sendJSON(code, data);
      }
      return;
    }
  } else if (router->server->uri().equals("/api/admin/lock")) {
    if (method == HTTP_POST) {
      if (router->server->hasArg("pw")) {
        res = lock(decodeURI(router->server->arg("pw")));
        data.add("res", res);
        code = 200;
      } else {
        data.add("err", "Expected 'pw' parameter");
        code = 422;
      }
      sendJSON(code, data);
      return;
    }
  } else if (router->server->uri().equals("/api/admin/unlock")) {
    if (method == HTTP_POST) {
      if (router->server->hasArg("pw")) {
        res = unlock(decodeURI(router->server->arg("pw")));
        data.add("res", res);
        sendJSON(200, data);
      } else {
        data.add("err", "Expected 'pw' parameter");
        sendJSON(422, data);
      }
      return;
    }
  } else if (router->server->uri().equals("/api/admin/file")) {
    if (method == HTTP_GET) {
      fileGet();
      return;
    } else if (method == HTTP_POST) {
      filePost();
      return;
    } else if (method == HTTP_DELETE) {
      fileDelete();
      return;
    }
  } else  if (router->server->uri().equals("/api")) {           // all cmd= types 
    if ((method == HTTP_GET) || (method == HTTP_POST)) {        // allow both
      if (server->arg("cmd")) {
        data.add("res", gc->doCommand(server->arg("cmd")));
        sendJSON(200, data);
        return;
      }
    }
  }
  Serial.printf("400 Invalid api request\n");
  data.add("err", "Invalid api request");
  sendJSON(400, data);
}
