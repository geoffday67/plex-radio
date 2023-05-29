#include <Arduino.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <VS1053.h>
#include <SPIFFS.h>
#include "../../XML/XmlParser/xml.h"
#include "vs1053b-patches-flac.h"

/*
Server[1]: IP address: 192.168.68.106, port: 32469, name: Plex Media Server:
Mac Mini
  -> controlURL:
ContentDirectory/73c5e1c3-8305-d645-c7db-95aa99f409ab/control.xml
*/

#define SERVER_IP "192.168.68.106"
#define SERVER_PORT 32469
#define SERVER_CONTROL_URL "ContentDirectory/73c5e1c3-8305-d645-c7db-95aa99f409ab/control.xml"

#define VS1053_CS 13
#define VS1053_DCS 25
#define VS1053_DREQ 26

#define VOLUME 100  // volume level 0-100

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

WiFiClient client;
WiFiUDP udp;
HTTPClient httpClient;

bool connectWiFi() {
  bool result = false;
  unsigned long start;

  WiFi.begin("Wario", "mansion1");
  Serial.print("Connecting ");

  start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 5000) {
      Serial.println();
      Serial.println("Timed out connecting to access point");
      goto exit;
    }
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  result = true;

exit:
  return result;
}

XmlParser browseParser, resultParser;

void resultCallback(char *pname, char *pvalue) {
  if (pvalue && *pvalue) {
    Serial.printf("%s = %s\n", pname, pvalue);
  }
}

void resultChar(char *pname, char c) {
  resultParser.processChar(c);
}

void browseCallback(char *pname, char *pvalue) {
  if (pvalue && *pvalue) {
    Serial.printf("%s = %s\n", pname, pvalue);
  }
  if (!strcmp(pname, "Result")) {
    if (!pvalue) {
      browseParser.setCharCallback(resultChar);
    } else {
      browseParser.setCharCallback(0);
    }
  }
}

void browse(char *pid) {
  String action("");
  char buffer[256];
  HTTPClient http_client;
  int c, count, n;

  Serial.printf("Browsing object %s at %s\n", pid, SERVER_CONTROL_URL);

  action += "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">";
  action += "<s:Body>";
  action += "<u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">";
  sprintf(buffer, "<ObjectID>%s</ObjectID>", pid);
  action += buffer;
  action += "<BrowseFlag>BrowseDirectChildren</BrowseFlag>";
  action += "<Filter>*</Filter>";
  action += "<StartingIndex>0</StartingIndex>";
  action += "<RequestedCount>10</RequestedCount>";
  action += "<SortCriteria></SortCriteria>";
  action += "</u:Browse>";
  action += "</s:Body>";
  action += "</s:Envelope>";

  sprintf(buffer, "http://%s:%d/%s", SERVER_IP, SERVER_PORT, SERVER_CONTROL_URL);
  http_client.begin(client, buffer);
  http_client.addHeader("SOAPACTION", "\"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"");
  int code = http_client.POST(action);

  browseParser.setXmlCallback(browseCallback);
  resultParser.setXmlCallback(resultCallback);

  while (client.available() == 0) {
    delay(100);
  }

  while (true) {
    count = client.available();
    if (count == 0) {
      break;
    }
    if (count > 256) {
      count = 256;
    }
    client.readBytes(buffer, count);
    for (n = 0; n < count; n++) {
      browseParser.processChar(buffer[n]);
    }
  }

  http_client.end();
}

void handleSearchLine(char *pline) {
  // Look for the Location: header

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

  if (!lwip_stricmp(pname, "location")) {
    // fetchDescription(pvalue);
  }
}

void playtest() {
  HTTPClient http_client;
  int n, count, buffer_size = 10240;
  byte *pbuffer;

  Serial.println("Requesting track");
  http_client.begin(client, "http://192.168.68.106:32469/object/25c5b27880aae2a04eed/file.flac");
  int code = http_client.GET();
  Serial.printf("Got code %d from track URL\n", code);

  pbuffer = new byte[buffer_size];

  while (client.available() == 0) {
    delay(100);
  }

  while (true) {
    count = client.available();
    if (count == 0) {
      Serial.println("No data available, waiting");
      delay(100);
      continue;
    }
    if (count > buffer_size) {
      Serial.println("Buffer limit reached");
      count = buffer_size;
    }
    client.readBytes(pbuffer, count);
    player.playChunk(pbuffer, count);
  }

  delete [] pbuffer;
  http_client.end();
  Serial.println("Finished playing");
}

bool initPlayer() {
  bool result = false;

  player.begin();

  if (player.isChipConnected()) {
    Serial.println("Chip connected");
  } else {
    Serial.println("Chip NOT connected");
  }

  player.switchToMp3Mode();
  player.loadUserCode(PATCH_FLAC, PATCH_FLAC_SIZE);
  player.setVolume(80);

  result = true;

exit:
  return result;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting");

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect();
  delay(1000);
  if (!connectWiFi()) {
    Serial.println("Error connecting WiFi");
    ESP.restart();
  } else {
    Serial.println("WiFi initialised");
  }

  SPI.begin();
  Serial.println("SPI initialised");

  if (initPlayer()) {
    Serial.println("VS1053 initialised");
  } else {
    Serial.println("Error initialising VS1053");
  }

  playtest();
  //browse("ab31bafbbb287b8e9bb9");
  return;

  if (udp.begin(1967)) {
    Serial.println("UDP listening");
  }

  char packet[] =
      "M-SEARCH * HTTP/1.1\nHOST: 239.255.255.250:1900\n"
      "MAN: \"ssdp:discover\"\n"
      "MX: 1\n"
      "ST: urn:schemas-upnp-org:device:MediaServer:1\n"
      "CPFN.UPNP.ORG: \"Plex radio\"\n"
      "\n";
  IPAddress server;
  server.fromString("239.255.255.250");
  udp.beginPacket(server, 1900);
  udp.write((uint8_t *)packet, strlen(packet));
  udp.endPacket();
}

void loop() {
  return;

  int length, line;
  char *ppacket, *pline, *pend;

  length = udp.parsePacket();
  if (length == 0) {
    return;
  }

  // Read the whole received packet into memory and parse afterwards so
  // there's no problem with re-using internet connections.
  ppacket = new char[length];
  udp.readBytes(ppacket, length);

  // Header lines end CR-LF so remove the CR by setting the null terminator.
  pline = ppacket;
  pend = strchr(pline, '\r');
  while (pend) {
    *pend = 0;
    if (*pline) {
      handleSearchLine(pline);
    }
    pline = pend + 2;
    pend = strchr(pline, '\r');
  }
  delete[] ppacket;
}