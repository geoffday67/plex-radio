#include "server.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

extern WiFiClient wifiClient;

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

void DLNAServer::browse(char *pid, int offset, int results, ObjectCallback objectCallback) {
  String action("");
  char buffer[256];
  HTTPClient http_client;
  int c, count, n, size, processed;
  unsigned long start;

  action += "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">";
  action += "<s:Body>";
  action += "<u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">";
  sprintf(buffer, "<ObjectID>%s</ObjectID>", pid);
  action += buffer;
  action += "<BrowseFlag>BrowseDirectChildren</BrowseFlag>";
  action += "<Filter>*</Filter>";
  action += "<StartingIndex>0</StartingIndex>";
  sprintf(buffer, "<RequestedCount>%d</RequestedCount>", results);
  action += buffer;
  action += "<SortCriteria></SortCriteria>";
  action += "</u:Browse>";
  action += "</s:Body>";
  action += "</s:Envelope>";

  sprintf(buffer, "%s%s", baseDomain, controlPath);
  ESP_LOGD(TAG, "Browsing object %s at %s", pid, buffer);
  http_client.setReuse(false);
  http_client.begin(wifiClient, buffer);
  http_client.addHeader("SOAPACTION", "\"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"");
  int code = http_client.POST(action);
  if (code != 200) {
    ESP_LOGW(TAG, "Got status %d browsing", code);
    goto exit;
  }

  size = http_client.getSize();

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

  processed = 0;
  while (processed < size) {
    start = millis();
    while ((count = wifiClient.available()) == 0) {
      if (millis() - start > 1000) {
        ESP_LOGW(TAG, "Timeout waiting for browsing response");
        goto exit;
      }
      delay(10);
    }
    if (count > 256) {
      count = 256;
    }
    wifiClient.readBytes(buffer, count);
    for (n = 0; n < count; n++) {
      pBrowseParser->processChar(buffer[n]);
      processed++;
    }
  }

exit:
  http_client.end();
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
  }
}