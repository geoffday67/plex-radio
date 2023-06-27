#pragma once

#include <WiFiUdp.h>

#include "../../XML/XmlParser/xml.h"
#include "server.h"

class SearchResult {
 public:
  SearchResult();
  ~SearchResult();
  int count;
  DLNAServer *pServers;

 private:
  void addServer(DLNAServer *);
  bool contains(DLNAServer *);

  friend class classDLNA;
};

class Service {
private:
  char type[128];  
  char controlURL[128];

  friend class classDLNA;
};

class classDLNA {
 public:
  classDLNA();
  SearchResult *findServers();

 private:
  WiFiUDP udp;
  XmlParser descriptionBrowser;

  static void parserCallback(char *, char *, void *);

  void handleSearchLine(char *, DLNAServer *);
  void fetchDescription(DLNAServer *);
  char *extractID(char *);
};

extern classDLNA DLNA;