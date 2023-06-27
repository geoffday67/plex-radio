#include "dlna.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include "../utils.h"

classDLNA DLNA;

extern WiFiClient wifiClient;

static const char *TAG = "DLNA";

SearchResult::SearchResult() {
  count = 0;
  pServers = 0;
}

SearchResult::~SearchResult() {
  delete[] pServers;
}

void SearchResult::addServer(DLNAServer *pserver) {
  int n;

  DLNAServer *pnew = new DLNAServer[count + 1];
  if (count > 0) {
    memcpy(pnew + 1, pServers, count * sizeof(DLNAServer));
  }
  memcpy(pnew, pserver, sizeof(DLNAServer));
  delete[] pServers;
  count++;
  pServers = pnew;
}

bool SearchResult::contains(DLNAServer *pserver) {
  int n;

  for (n = 0; n < count; n++) {
    if (!strcmp(pserver->id, pServers[n].id)) {
      return true;
    }
  }

  return false;
}

classDLNA::classDLNA() {
}

SearchResult *classDLNA::findServers() {
  int length;
  char *ppacket, *pline, *pend;

  char packet[] =
      "M-SEARCH * HTTP/1.1\n"
      "HOST: 239.255.255.250:1900\n"
      "MAN: \"ssdp:discover\"\n"
      "MX: 3\n"
      "ST: urn:schemas-upnp-org:service:ContentDirectory:1\n"
      "CPFN.UPNP.ORG: \"Plex radio\"\n"
      "\n";
  udp.beginPacket("239.255.255.250", 1900);
  udp.write((uint8_t *)packet, strlen(packet));
  udp.endPacket();

  SearchResult *presult = new SearchResult;

  // Receive packets for 3 seconds.
  unsigned long start = millis();
  while (millis() - start < 3000) {
    yield;

    length = udp.parsePacket();
    if (length == 0) {
      continue;
    }

    ESP_LOGD(TAG, "UDP response received from %s", udp.remoteIP().toString().c_str());

    ppacket = new char[length];
    udp.readBytes(ppacket, length);
    DLNAServer *pserver = new DLNAServer;

    pline = ppacket;
    pend = strchr(pline, '\r');
    while (pend) {
      *pend = 0;
      if (*pline) {
        handleSearchLine(pline, pserver);
      }
      pline = pend + 1;
      if (*pline == '\n') {
        pline++;
      }
      pend = strchr(pline, '\r');
    }
    delete[] ppacket;

    // We should have location and id now, check if we've seen it before, if not add it to the result set.
    if (presult->contains(pserver)) {
      ESP_LOGD(TAG, "Duplicate ID found: %s", pserver->id);
      delete pserver;
      continue;
    }

    fetchDescription(pserver);
    ESP_LOGD(TAG, "Got new server %s", pserver->name);
    presult->addServer(pserver);
  }

  return presult;
}

void classDLNA::handleSearchLine(char *pline, DLNAServer *pserver) {
  char *pid;

  char *pname = pline;
  char *pseparator = strchr(pline, ':');
  if (!pseparator) {
    return;
  }
  *pseparator = 0;
  char *pvalue = pname + strlen(pname) + 1;
  while (isWhitespace(*pvalue)) {
    pvalue++;
  }

  if (!strcasecmp(pname, "Location")) {
    strcpy(pserver->descriptionURL, pvalue);
  } else if (!strcasecmp(pname, "USN")) {
    pid = extractID(pvalue);
    strcpy(pserver->id, pid);
  }
}

char *classDLNA::extractID(char *pvalue) {
  if (strncasecmp(pvalue, "uuid:", 5)) {
    return 0;
  }
  char *pstart = pvalue + 5;
  char *pend = strchr(pstart, ':');
  if (pend) {
    *pend = 0;
  }
  return pstart;
}

void classDLNA::fetchDescription(DLNAServer *pserver) {
  HTTPClient http_client;
  int c, size, processed;
  unsigned long start;
  char *pdomain;

  ESP_LOGD(TAG, "Fetching description from %s", pserver->descriptionURL);

  http_client.setReuse(false);  // Prevents each call using memory to maintain state.
  http_client.begin(wifiClient, pserver->descriptionURL);

  // Extract scheme and domain from description location as subsequent URLs are relative to this.
  pdomain = strnthchr(pserver->descriptionURL, '/', 3);
  if (pdomain) {
    *pdomain = 0;
    strcpy(pserver->baseDomain, pserver->descriptionURL);
  }
  ESP_LOGD(TAG, "Base domain is %s", pserver->baseDomain);

  int code = http_client.GET();
  if (code != 200) {
    ESP_LOGW(TAG, "Got status %d fetching description", code);
    goto exit;
  }
  size = http_client.getSize();

  descriptionBrowser.reset();
  descriptionBrowser.setXmlCallback(parserCallback);
  descriptionBrowser.setData(pserver);

  processed = 0;
  while (processed < size) {
    start = millis();
    while (wifiClient.available() == 0) {
      if (millis() - start > 1000) {
        ESP_LOGW(TAG, "Timeout waiting for description response");
        goto exit;
      }
      delay(10);
    }
    c = wifiClient.read();
    if (c != -1) {
      descriptionBrowser.processChar(c);
      processed++;
    }
  }

exit:
  http_client.end();
}

void classDLNA::parserCallback(char *pname, char *pvalue, void *pdata) {
  static Service *pservice;

  if (!strcasecmp(pname, "modelname") && pvalue) {
    DLNAServer *pserver = (DLNAServer *)pdata;
    strcpy(pserver->name, pvalue);
  } else if (!strcasecmp(pname, "service") && !pvalue) {
    // A service block is starting, get its type and control URL.
    pservice = new Service;
  } else if (!strcasecmp(pname, "service") && pvalue) {
    if (begins(pservice->type, "urn:schemas-upnp-org:service:ContentDirectory")) {
      DLNAServer *pserver = (DLNAServer *)pdata;
      strcpy(pserver->controlPath, pservice->controlURL);
    }
    delete pservice;
  } else if (!strcasecmp(pname, "serviceType") && pvalue) {
    if (pservice) {
      strcpy(pservice->type, pvalue);
    }
  } else if (!strcasecmp(pname, "controlURL") && pvalue) {
    if (pservice) {
      strcpy(pservice->controlURL, pvalue);
    }
  }
}
