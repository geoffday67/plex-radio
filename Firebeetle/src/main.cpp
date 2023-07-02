#include <Arduino.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <VS1053.h>
#include <WiFi.h>
#include <sqlite3.h>

#include <regex>

#include "../../XML/XmlParser/xml.h"
#include "data/album.h"
#include "data/data.h"
#include "data/track.h"
#include "dlna/dlna.h"
#include "rotary.h"
#include "screens/Albums.h"
#include "screens/Output.h"
#include "vs1053b-patches-flac.h"

#define ENCODER_CLK 16
#define ENCODER_DT 17
#define ENCODER_BUTTON 4

#define VS1053_CS 13
#define VS1053_DCS 25
#define VS1053_DREQ 26
#define VOLUME 100  // volume level 0-100

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

WiFiClient wifiClient;
HTTPClient httpClient;
sqlite3 *database;
Rotary encoder(ENCODER_CLK, ENCODER_DT, ENCODER_BUTTON);

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

void playtest() {
  HTTPClient http_client;
  int n, count, buffer_size = 10240;
  byte *pbuffer;

  Serial.println("Requesting track");
  http_client.begin(wifiClient, "http://192.168.68.106:32469/object/25c5b27880aae2a04eed/file.flac");
  int code = http_client.GET();
  Serial.printf("Got code %d from track URL\n", code);

  pbuffer = new byte[buffer_size];

  while (wifiClient.available() == 0) {
    delay(100);
  }

  while (true) {
    count = wifiClient.available();
    if (count == 0) {
      Serial.println("No data available, waiting");
      delay(100);
      continue;
    }
    if (count > buffer_size) {
      Serial.println("Buffer limit reached");
      count = buffer_size;
    }
    wifiClient.readBytes(pbuffer, count);
    player.playChunk(pbuffer, count);
  }

  delete[] pbuffer;
  http_client.end();
  Serial.println("Finished playing");
}

void fileTest() {
  int count;
  uint8_t buffer[1024];

  SPIFFS.begin();
  File file = SPIFFS.open("/bbc.ts", "r");
  Serial.printf("File size = %d\n", file.size());

  count = file.readBytes((char *)buffer, 1024);
  while (count > 0) {
    player.playChunk(buffer, count);
    count = file.readBytes((char *)buffer, 1024);
  }

  Serial.println("Player finished");
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

DLNAServer *pPlex;

void onServerFound(DLNAServer *pserver) {
  Serial.printf("Server found: %s\n", pserver->name);
  delete pPlex;
  pPlex = new DLNAServer(*pserver);
}

void onObjectFound(Object *pobject) {
  static std::regex regexYear(" \\(\\d\\d\\d\\d\\)$");
  static std::cmatch matchYear;

  Album *palbum = new Album;
  char *pname, *pseparator;

  strlcpy(palbum->id, pobject->id, ID_SIZE);

  pname = strdup(pobject->name);

  std::regex_search(pname, matchYear, regexYear);
  if (!matchYear.empty()) {
    pname[matchYear.position(0)] = 0;
  }

  pseparator = strstr(pname, " - ");
  if (pseparator) {
    *pseparator = 0;
    strlcpy(palbum->artist, pname, ARTIST_SIZE);
    strlcpy(palbum->title, pseparator + 3, TITLE_SIZE);
  } else {
    strlcpy(palbum->title, pname, TITLE_SIZE);
  }

  free(pname);

  Serial.printf("Found album %s\n", palbum->title);
  Data.storeAlbum(palbum);
  delete palbum;
}

void setup() {
  int n, t;

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

  if (LittleFS.begin(true)) {
    Serial.println("LittleFS initialised");
  } else {
    Serial.println("Error initialising LittleFS");
  }
  Serial.printf("Total bytes in file system: %d\n", LittleFS.totalBytes());

  /*if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialised");
  } else {
    Serial.println("Error initialising SPIFFS");
  }*/

  Output.begin();
  Serial.println("Output initialised");
  Output.addText(0, 0, "Geoff is great!");

  Data.begin();
  Serial.println("Database initialised");

  encoder.begin();
  Serial.println("Encoder initialised");

  Albums.activate();
  return;

  /*SPI.begin();
  Serial.println("SPI initialised");

  if (initPlayer()) {
    Serial.println("VS1053 initialised");
  } else {
    Serial.println("Error initialising VS1053");
  }*/

  Data.clearAll();

  Serial.printf("Free memory before: %d\n", esp_get_free_heap_size());

  Serial.println("Finding servers");
  DLNA.findServers(onServerFound);
  Serial.printf("Free memory: %d\n", esp_get_free_heap_size());

  Serial.printf("\nPlex server name: %s\n\n", pPlex->name);

  pPlex->browse("9b55ecd3e74c09febffe", 0, 50, onObjectFound);
  Serial.printf("Free memory: %d\n", esp_get_free_heap_size());

  /*for (n = 0; n < presult->count; n++) {
    Serial.printf("Found %s, id %s, control %s%s\n", (presult->pServers + n)->name, (presult->pServers + n)->id, (presult->pServers + n)->baseDomain, (presult->pServers + n)->controlPath);
  }

  DLNAServer *pPlex = presult->pServers;
  BrowseResult *pbrowse = pPlex->browse("9b55ecd3e74c09febffe", 0, 10);
  Serial.printf("%d objects found\n", pbrowse->count);

  Album *palbums = new Album[pbrowse->count], *palbum = palbums;
  char *pname, *pseparator;
  std::regex regexYear(" \\(\\d\\d\\d\\d\\)$");
  std::cmatch matchYear;
  for (n = 0; n < pbrowse->count; n++) {
    strlcpy(palbum->id, (pbrowse->pObjects + n)->id, ID_SIZE);

    pname = strdup((pbrowse->pObjects + n)->name);

    std::regex_search(pname, matchYear, regexYear);
    if (!matchYear.empty()) {
      pname[matchYear.position(0)] = 0;
    }

    pseparator = strstr(pname, " - ");
    if (pseparator) {
      *pseparator = 0;
      strlcpy(palbum->artist, pname, ARTIST_SIZE);
      strlcpy(palbum->title, pseparator + 3, TITLE_SIZE);
    } else {
      strlcpy(palbum->title, pname, TITLE_SIZE);
    }

    free(pname);

    // Get track details for album.
    BrowseResult *pTrackBrowse = pPlex->browse(palbum->id, 0, 100);
    Track *ptracks = new Track[pTrackBrowse->count], *ptrack = ptracks;
    for (t = 0; t < pTrackBrowse->count; t++) {
      strlcpy(ptrack->id, (pTrackBrowse->pObjects + t)->id, ID_SIZE);
      strlcpy(ptrack->title, (pTrackBrowse->pObjects + t)->name, TITLE_SIZE);
      strlcpy(ptrack->resource, (pTrackBrowse->pObjects + t)->resource, RESOURCE_SIZE);
      ptrack++;
    }
    Serial.printf("Got %d tracks for album %s\n", pTrackBrowse->count, palbum->title);
    delete pTrackBrowse;
    // Data.storeTracks(ptracks, pTrackBrowse->count);
    delete[] ptracks;
    Serial.printf("Free memory: %d\n", esp_get_free_heap_size());

    palbum++;
  }
  Data.storeAlbums(palbums, pbrowse->count);
  delete[] palbums;

  delete pbrowse;
  delete presult;*/

  // Data.dumpDatabase();
}

void loop() {
  int direction;

  if (direction = encoder.getDirection()) {
    EventManager.queueEvent(new EncoderEvent(direction));
  }

  EventManager.processEvents();
}