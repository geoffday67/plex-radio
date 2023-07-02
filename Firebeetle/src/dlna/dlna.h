#pragma once

#include <WiFiUdp.h>

#include "../../XML/XmlParser/xml.h"
#include "server.h"

class Service {
 private:
  char type[128];
  char controlURL[128];

  friend class classDLNA;
};

typedef void (*ServerCallback)(DLNAServer *);

class classDLNA {
 public:
  void findServers(ServerCallback serverCallback);

 private:
  WiFiUDP udp;
  XmlParser descriptionBrowser;

  static void parserCallback(char *, char *, void *);

  void handleSearchLine(char *, DLNAServer *);
  void fetchDescription(DLNAServer *);
  char *extractID(char *);
};

extern classDLNA DLNA;