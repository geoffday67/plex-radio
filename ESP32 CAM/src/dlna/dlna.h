#pragma once

#include "esp_http_client.h"

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
  XmlParser descriptionBrowser;

  static esp_err_t httpEventHandler(esp_http_client_event_t*);
  static void parserCallback(char *, char *, void *);

  void handleSearchLine(char *, DLNAServer *);
  void fetchDescription(DLNAServer *);
  char *extractID(char *);
};

extern classDLNA DLNA;