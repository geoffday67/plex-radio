#include "server.h"

#include "esp32-hal-log.h"
#include "esp_http_client.h"

static const char *TAG = "SERVER";

DLNAServer::DLNAServer() {
  id[0] = 0;
  name[0] = 0;
  descriptionURL[0] = 0;
  controlPath[0] = 0;
  baseDomain[0] = 0;
  pBrowseParser = 0;
  pResultParser = 0;
}

DLNAServer::DLNAServer(DLNAServer &src) {
  strcpy(id, src.id);
  strcpy(name, src.name);
  strcpy(descriptionURL, src.descriptionURL);
  strcpy(controlPath, src.controlPath);
  strcpy(baseDomain, src.baseDomain);
  pBrowseParser = 0;
  pResultParser = 0;
}

DLNAServer::~DLNAServer() {
  delete pBrowseParser;
  delete pResultParser;
}

void DLNAServer::browse(char *pid, int offset, int results, char *pfilter, int *pfound, ObjectCallback objectCallback) {
  char buffer[256], *paction;
  esp_http_client_handle_t client;
  esp_http_client_config_t config;
  int n, size, code, length;
  char *presponse = 0;

  paction = new char[1024];

  strcpy(paction, "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">");
  strcat(paction, "<s:Body>");
  strcat(paction, "<u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">");
  sprintf(buffer, "<ObjectID>%s</ObjectID>", pid);
  strcat(paction, buffer);

  strcat(paction, "<BrowseFlag>BrowseDirectChildren</BrowseFlag>");

  sprintf(buffer, "<Filter>%s</Filter>", pfilter);
  strcat(paction, buffer);

  sprintf(buffer, "<StartingIndex>%d</StartingIndex>", offset);
  strcat(paction, buffer);

  sprintf(buffer, "<RequestedCount>%d</RequestedCount>", results);
  strcat(paction, buffer);
  strcat(paction, "<SortCriteria></SortCriteria>");
  strcat(paction, "</u:Browse>");
  strcat(paction, "</s:Body>");
  strcat(paction, "</s:Envelope>");

  sprintf(buffer, "%s%s", baseDomain, controlPath);
  ESP_LOGD(TAG, "Browsing object %s at %s", pid, buffer);

  memset(&config, 0, sizeof config);
  config.url = buffer;
  config.user_agent = "Plex radio";
  config.method = HTTP_METHOD_POST;
  client = esp_http_client_init(&config);
  esp_http_client_set_header(client, "SOAPACTION", "\"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"");

  // TODO Timeout
  // TODO Connection re-use

  length = strlen(paction);
  esp_http_client_open(client, length);
  esp_http_client_write(client, paction, length);
  size = esp_http_client_fetch_headers(client);
  code = esp_http_client_get_status_code(client);
  ESP_LOGD(TAG, "Status code %d, content length %d", code, size);
  if (code != 200) {
    esp_http_client_close(client);
    goto exit;
  }
  presponse = new char[size + 1];
  n = esp_http_client_read(client, presponse, size);
  esp_http_client_close(client);

  if (!pBrowseParser) {
    pBrowseParser = new XmlParser();
  }
  pBrowseParser->reset();
  pBrowseParser->setXmlCallback(browseCallback);
  pBrowseParser->setData(this);

  if (!pResultParser) {
    pResultParser = new XmlParser();
  }
  pResultParser->reset();
  pResultParser->setXmlCallback(resultCallback);
  pResultParser->setData(this);

  this->objectCallback = objectCallback;

  for (n = 0; n < size; n++) {
    pBrowseParser->processChar(presponse[n]);
  }

  if (pfound) {
    *pfound = found;
  }

exit:
  delete[] paction;
  delete[] presponse;
  esp_http_client_cleanup(client);
}

void DLNAServer::resultCallback(char *pname, char *pvalue, void *pdata) {
  DLNAServer *pserver = (DLNAServer *)pdata;
  static Object *pobject;

  if (!strcasecmp(pname, "container") || !strcasecmp(pname, "item")) {
    if (pvalue) {
      (*pserver->objectCallback)(pobject);
      delete pobject;
    } else {
      pobject = new Object;
    }
  } else if (!strcasecmp(pname, "dc:title") && pvalue) {
    strlcpy(pobject->name, pvalue, NAME_SIZE);
  } else if (!strcasecmp(pname, "id") && pvalue) {
    strlcpy(pobject->id, pvalue, ID_SIZE);
  } else if (!strcasecmp(pname, "res") && pvalue) {
    strlcpy(pobject->resource, pvalue, RESOURCE_SIZE);
  }
}

void DLNAServer::resultChar(char *pname, char c, void *pdata) {
  DLNAServer *pserver = (DLNAServer *)pdata;

  pserver->pResultParser->processChar(c);
}

void DLNAServer::browseCallback(char *pname, char *pvalue, void *pdata) {
  DLNAServer *pserver = (DLNAServer *)pdata;

  if (!strcmp(pname, "Result")) {
    if (!pvalue) {
      pserver->pBrowseParser->setCharCallback(resultChar);
    } else {
      pserver->pBrowseParser->setCharCallback(0);
    }
  } else if (!strcmp(pname, "NumberReturned")) {
    if (pvalue) {
      pserver->found = atoi(pvalue);
    }
  }
}