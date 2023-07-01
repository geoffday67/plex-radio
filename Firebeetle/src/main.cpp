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
#include "dlna/dlna.h"
#include "screens/Albums.h"
#include "screens/Output.h"
#include "vs1053b-patches-flac.h"

#define VS1053_CS 13
#define VS1053_DCS 25
#define VS1053_DREQ 26
#define VOLUME 100  // volume level 0-100

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

WiFiClient wifiClient;
HTTPClient httpClient;
sqlite3 *database;

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

int sqlCallback(void *data, int argc, char **argv, char **azColName) {
  int i;

  for (i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  Serial.println();

  return 0;
}

void addAlbum(Object *pobject) {
  int rc;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "INSERT INTO albums (id, title) VALUES (?, ?)", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album preparation: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 1, pobject->id, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_bind_text(stmt, 2, pobject->name, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during album binding: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during album step: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

exit:
  sqlite3_finalize(stmt);
}

void setup() {
  int n;

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

  /*SPI.begin();
  Serial.println("SPI initialised");

  if (initPlayer()) {
    Serial.println("VS1053 initialised");
  } else {
    Serial.println("Error initialising VS1053");
  }*/

  // fileTest();
  // playtest();
  // browse("ab31bafbbb287b8e9bb9");
  // return;

  SearchResult *presult = DLNA.findServers();
  for (n = 0; n < presult->count; n++) {
    Serial.printf("Found %s, id %s, control %s%s\n", (presult->pServers + n)->name, (presult->pServers + n)->id, (presult->pServers + n)->baseDomain, (presult->pServers + n)->controlPath);
  }

  DLNAServer *pPlex = presult->pServers;
  BrowseResult *pbrowse = pPlex->browse("9b55ecd3e74c09febffe", 0, 30);
  Serial.printf("%d objects found\n", pbrowse->count);
  Album *palbums = new Album[pbrowse->count], *palbum = palbums;
  char *pname, *pseparator;
  std::regex regexYear(" \\(\\d\\d\\d\\d\\)$");
  std::cmatch matchYear;
  for (n = 0; n < pbrowse->count; n++) {
    // Serial.printf("Found object %s, ID %s, resource %s\n", (pbrowse->pObjects + n)->name, (pbrowse->pObjects + n)->id, (pbrowse->pObjects + n)->resource);
    // Convert the objects into albums.
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

    palbum++;
  }
  Data.storeAlbums(palbums, pbrowse->count);

  delete pbrowse;
  delete presult;

  Data.dumpDatabase();
}

void loop() {
}