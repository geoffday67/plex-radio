#include "server.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

extern WiFiClient wifiClient;

static const char *TAG = "SERVER";

BrowseResult::BrowseResult() {
  count = 0;
  pObjects = 0;
}

BrowseResult::~BrowseResult() {
  delete[] pObjects;
}

void BrowseResult::addObject(Object *pobject) {
  int n;

  Object *pnew = new Object[count + 1];
  if (count > 0) {
    memcpy(pnew, pObjects, count * sizeof(Object));
  }
  memcpy(pnew + count, pobject, sizeof(Object));
  delete[] pObjects;
  count++;
  pObjects = pnew;
}

DLNAServer::DLNAServer() {
  id[0] = 0;
  name[0] = 0;
  descriptionURL[0] = 0;
  controlPath[0] = 0;
  baseDomain[0] = 0;
}

BrowseResult *DLNAServer::browse(char *pid, int offset, int results) {
  String action("");
  char buffer[256];
  HTTPClient http_client;
  int c, count, n, size, processed;
  unsigned long start;

  pBrowseResult = new BrowseResult;

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
  http_client.begin(wifiClient, buffer);
  http_client.addHeader("SOAPACTION", "\"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"");
  int code = http_client.POST(action);
  if (code != 200) {
    ESP_LOGW(TAG, "Got status %d browsing", code);
    goto exit;
  }

  size = http_client.getSize();

  browseParser.reset();
  browseParser.setXmlCallback(browseCallback);
  browseParser.setData(this);

  resultParser.reset();
  resultParser.setXmlCallback(resultCallback);
  resultParser.setData(this);

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
      browseParser.processChar(buffer[n]);
      processed++;
    }
  }

exit:
  http_client.end();

  return pBrowseResult;
}

void DLNAServer::resultCallback(char *pname, char *pvalue, void *pdata) {
  DLNAServer *pserver = (DLNAServer *)pdata;
  static Object *pobject;

  /*if (pname && pvalue && *pvalue) {
    Serial.printf("%s = %s\n", pname, pvalue);
  }*/

  if (!strcasecmp(pname, "container") || !strcasecmp(pname, "item")) {
    if (pvalue) {
      pserver->pBrowseResult->addObject(pobject);
    } else {
      pobject = new Object;
    }
  } else if (!strcasecmp(pname, "dc:title") && pvalue) {
    strcpy(pobject->name, pvalue);
  } else if (!strcasecmp(pname, "id") && pvalue) {
    strcpy(pobject->id, pvalue);
  } else if (!strcasecmp(pname, "res") && pvalue) {
    strcpy(pobject->resource, pvalue);
  }
}

void DLNAServer::resultChar(char *pname, char c, void *pdata) {
  DLNAServer *pserver = (DLNAServer *)pdata;

  pserver->resultParser.processChar(c);
}

void DLNAServer::browseCallback(char *pname, char *pvalue, void *pdata) {
  DLNAServer *pserver = (DLNAServer *)pdata;

  if (!strcmp(pname, "Result")) {
    if (!pvalue) {
      pserver->browseParser.setCharCallback(resultChar);
    } else {
      pserver->browseParser.setCharCallback(0);
    }
  }
}