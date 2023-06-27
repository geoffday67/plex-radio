#include <Arduino.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <VS1053.h>
#include <WiFi.h>
#include <sqlite3.h>

#include "../../XML/XmlParser/xml.h"
#include "dlna/dlna.h"
#include "vs1053b-patches-flac.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

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

void dumpDatabase() {
  int rc;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(database, "SELECT * FROM albums", -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Error during preparation: ", sqlite3_errmsg(database));
    goto exit;
  }
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    const unsigned char *id = sqlite3_column_text(stmt, 0);
    const unsigned char *title = sqlite3_column_text(stmt, 1);
    Serial.printf("ID %s: %s\n", id, title);
  }
  if (rc != SQLITE_DONE) {
    Serial.printf("Error during steps: ", sqlite3_errmsg(database));
  }

exit:
  sqlite3_finalize(stmt);
}

bool initDatabase() {
  bool result = false;
  int rc;

  SPIFFS.remove("/plex.db"); // For now remove the database at each run.

  rc = sqlite3_initialize();
  if (rc != SQLITE_OK) {
    Serial.printf("Can't initialise database, error code: %d\n", rc);
    goto exit;
  }

  rc = sqlite3_open("/spiffs/plex.db", &database);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  rc = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS albums (id TEXT PRIMARY KEY, title TEXT)", NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Can't create albums table: %s\n", sqlite3_errmsg(database));
    goto exit;
  }

  result = true;

exit:
  return result;
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

  if (SPIFFS.begin()) {
    Serial.println("SPIFFS initialised");
  } else {
    Serial.println("Error initialising SPIFFS");
  }

  if (initDatabase()) {
    Serial.println("Database initialised");
  } else {
    Serial.println("Error initialising database");
  }

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

  // If there's a Plex server found then use its music ID.
  // IDs are permanent so we can use a fixed value.

  /*
  Get list of albums from server (any order).
  Add each to database. (Instead of creating a list?)
  Query database (with ordering and filtering?) to get screen contents on the album list screen.
  */

  DLNAServer *pPlex = presult->pServers;
  BrowseResult *pbrowse = pPlex->browse("9b55ecd3e74c09febffe", 0, 10);
  Serial.printf("%d objects found\n", pbrowse->count);
  for (n = 0; n < pbrowse->count; n++) {
    //Serial.printf("Found object %s, ID %s, resource %s\n", (pbrowse->pObjects + n)->name, (pbrowse->pObjects + n)->id, (pbrowse->pObjects + n)->resource);
    addAlbum(pbrowse->pObjects + n);
  }

  delete pbrowse;
  delete presult;

  dumpDatabase();
}

void loop() {
}