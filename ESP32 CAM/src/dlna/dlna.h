#pragma once

#include "esp_http_client.h"

#include "../../XML/XmlParser/xml.h"
#include "server.h"

class Service {
 private:
  char type[SERVICE_TYPE_SIZE];
  char controlURL[URL_SIZE];

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