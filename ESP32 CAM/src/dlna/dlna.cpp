#include <Arduino.h>

#include "dlna.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include "../utils.h"
#include "esp_log.h"

classDLNA DLNA;

static const char *TAG = "DLNA";

void classDLNA::findServers(ServerCallback serverCallback) {
  int client, length;
  struct sockaddr_in dest_addr, src_addr;
  char *pbuffer;
  char *pline, *pend;

  // TODO Receive multiple responses for 5(?) seconds, at the moment only the first response is used.

  client = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

  inet_pton(AF_INET, "239.255.255.250", &dest_addr.sin_addr);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(1900);

  char packet[] =
      "M-SEARCH * HTTP/1.1\n"
      "HOST: 239.255.255.250:1900\n"
      "MAN: \"ssdp:discover\"\n"
      "MX: 3\n"
      "ST: urn:schemas-upnp-org:service:ContentDirectory:1\n"
      "CPFN.UPNP.ORG: \"Plex radio\"\n"
      "\n";

  length = sendto(client, packet, strlen(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  ESP_LOGD(TAG, "%d bytes sent", length);

  pbuffer = new char[2048];
  socklen_t src_length = sizeof src_addr;
  length = recvfrom(client, pbuffer, 2048, 0, (sockaddr *)&src_addr, &src_length);
  pbuffer[length] = 0;
  ESP_LOGD(TAG, "%d bytes received from %s", length, inet_ntoa(src_addr.sin_addr));

  DLNAServer *pserver = new DLNAServer;

  pline = pbuffer;
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
  delete[] pbuffer;

  // We should have location and id now, check if we've seen it before, if not add it to the result set.
  /*if (presult->contains(pserver)) {
    ESP_LOGD(TAG, "Duplicate ID found: %s", pserver->id);
    delete pserver;
    continue;
  }*/

  fetchDescription(pserver);
  ESP_LOGD(TAG, "Got new server %s", pserver->name);
  (*serverCallback)(pserver);
  delete pserver;
}

esp_err_t classDLNA::httpEventHandler(esp_http_client_event_t *evt) {
  return ESP_OK;
}

void classDLNA::handleSearchLine(char *pline, DLNAServer *pserver) {
  char *pid;

  ESP_LOGI(TAG, "Processing line %s", pline);

  char *pname = pline;
  char *pseparator = strchr(pline, ':');
  if (!pseparator) {
    return;
  }
  *pseparator = 0;
  char *pvalue = pname + strlen(pname) + 1;
  while (isspace(*pvalue)) {
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
  esp_http_client_handle_t client;
  esp_http_client_config_t config;
  int n, size, code;
  char *pdomain, *presponse = 0;

  ESP_LOGD(TAG, "Fetching description from %s", pserver->descriptionURL);

  memset(&config, 0, sizeof config);
  config.url = pserver->descriptionURL;
  config.user_agent = "Plex radio";
  config.method = HTTP_METHOD_GET;
  config.event_handler = httpEventHandler;
  client = esp_http_client_init(&config);

  // Extract scheme and domain from description location as subsequent URLs are relative to this.
  pdomain = strnthchr(pserver->descriptionURL, '/', 3);
  if (pdomain) {
    *pdomain = 0;
    strcpy(pserver->baseDomain, pserver->descriptionURL);
  }
  ESP_LOGD(TAG, "Base domain is %s", pserver->baseDomain);

  // TODO Timeout
  esp_http_client_open(client, 0);
  size = esp_http_client_fetch_headers(client);
  code = esp_http_client_get_status_code(client);
  ESP_LOGD(TAG, "Status code %d, content length %d", code, size);
  if (code != 200) {
    ESP_LOGW(TAG, "Got status %d fetching description", code);
    esp_http_client_close(client);
    goto exit;
  }
  presponse = new char[size + 1];
  n = esp_http_client_read(client, presponse, size);
  ESP_LOGD(TAG, "%d bytes read", n);
  esp_http_client_close(client);

  descriptionBrowser.reset();
  descriptionBrowser.setXmlCallback(parserCallback);
  descriptionBrowser.setData(pserver);

  for (n = 0; n < size; n++) {
    descriptionBrowser.processChar(presponse[n]);
  }

exit:
  esp_http_client_cleanup(client);
}

void classDLNA::parserCallback(char *pname, char *pvalue, void *pdata) {
  static Service *pservice;

  if (!strcasecmp(pname, "modelname") && pvalue) {
    DLNAServer *pserver = (DLNAServer *)pdata;
    strcpy(pserver->name, pvalue);
    ESP_LOGI(TAG, "Server name found: %s", pvalue);
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
      ESP_LOGI(TAG, "Service type found: %s", pvalue);
    }
  } else if (!strcasecmp(pname, "controlURL") && pvalue) {
    if (pservice) {
      strcpy(pservice->controlURL, pvalue);
      ESP_LOGI(TAG, "Control URL found: %s", pvalue);
    }
  }
}
