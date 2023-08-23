#include <Arduino.h>
#include <Debouncer.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <sqlite3.h>

#include <regex>

#include "../../XML/XmlParser/xml.h"
#include "data/album.h"
#include "data/data.h"
#include "data/track.h"
#include "dlna/dlna.h"
#include "player.h"
#include "rotary.h"
#include "screens/Albums.h"
#include "screens/Output.h"

#define ARDUINO_STACK 8192
#define ARDUINO_PRIORITY 1
#define ARDUINO_CORE 1

#define ENCODER_CLK 16
#define ENCODER_DT 17
#define ENCODER_BUTTON 4

#define VOLUME_PIN 36
#define VOLUME 100  // volume level 0-100

WiFiClient wifiClient;
HTTPClient httpClient;
sqlite3 *database;
Rotary encoder(ENCODER_CLK, ENCODER_DT, ENCODER_BUTTON);
volatile bool wifiConnected = false;
volatile bool playerReady = false;

int getEncoder() {
  return digitalRead(ENCODER_BUTTON);
}

void encoderChanged(int value) {
  EventManager.queueEvent(new SwitchEvent(value == 0));
}

Debouncer encoderDebouncer(getEncoder, encoderChanged, 100);

void volumeTask(void *pdata) {
  int volume;
  static int previousVolume = -1;

  while (true) {
    volume = analogRead(VOLUME_PIN) * 100 / 4095;
    if (volume != previousVolume) {
      if (playerReady) {
        Player.setVolume(volume);
        Serial.printf("New volume %d\n", volume);
      }
      previousVolume = volume;
    }

  taskYIELD();
  }
}

void volumeBegin() {
  xTaskCreatePinnedToCore(
      volumeTask,
      "Volumn",
      ARDUINO_STACK,
      NULL,
      ARDUINO_PRIORITY,
      NULL,
      ARDUINO_CORE);
}

void playerBeginTask(void *pdata) {
  if (Player.begin()) {
    playerReady = true;
    Serial.println("Player initialised");
  } else {
    Serial.println("Error initialisin player");
  }

  vTaskDelete(NULL);
}

void playerBegin() {
  xTaskCreatePinnedToCore(
      playerBeginTask,
      "Player begin",
      ARDUINO_STACK * 2,
      NULL,
      ARDUINO_PRIORITY,
      NULL,
      ARDUINO_CORE);
}

void playerWait() {
  while (!playerReady) {
    delay(250);
  }
}

void connectWiFiTask(void *pdata) {
  unsigned long start;

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin("Wario", "mansion1");
    Serial.print("Connecting ");

    start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - start > 5000) {
        Serial.println();
        Serial.println("Timed out connecting to access point");
        break;
      }
      delay(100);
      Serial.print(".");
    }
  }

  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  wifiConnected = true;

  vTaskDelete(NULL);
}

void wifiBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);
  xTaskCreatePinnedToCore(
      connectWiFiTask,
      "Connect WiFi",
      ARDUINO_STACK,
      NULL,
      ARDUINO_PRIORITY,
      NULL,
      ARDUINO_CORE);
}

void wifiWait() {
  while (!wifiConnected) {
    delay(250);
  }
}

DLNAServer *pPlex;

void onServerFound(DLNAServer *pserver) {
  Serial.printf("Server found: %s\n", pserver->name);
  // TODO Check that this is a Plex server, and maybe that we haven't seen it before.
  delete pPlex;
  pPlex = new DLNAServer(*pserver);
}

void onAlbumFound(Object *pobject) {
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

  Data.storeAlbum(palbum);
  Serial.printf("Stored album %s\n", palbum->title);
  delete palbum;
}

char albumId[ID_SIZE];

void onTrackFound(Object *pobject) {
  Track *ptrack = new Track;
  strlcpy(ptrack->id, pobject->id, ID_SIZE);
  strlcpy(ptrack->title, pobject->name, TITLE_SIZE);
  strlcpy(ptrack->resource, pobject->resource, RESOURCE_SIZE);
  strlcpy(ptrack->album, albumId, ID_SIZE);
  Data.storeTrack(ptrack);
  Serial.printf("Stored track: %s\n", pobject->name);
  delete ptrack;
}

void refreshData() {
  int n, count;

  Data.clearAll();
  Serial.println("Database cleared");

  pPlex->browse("9b55ecd3e74c09febffe", 0, 20, onAlbumFound);
  Serial.printf("Free memory: %d\n", esp_get_free_heap_size());

  Album *palbums, *palbum;
  count = Data.getAlbums(&palbums);
  Serial.printf("%d albums fetched\n", count);
  for (n = 0; n < count; n++) {
    palbum = palbums + n;
    Serial.printf("Album %s, %s BY %s\n", palbum->id, palbum->title, palbum->artist);
    strcpy(albumId, palbum->id);
    pPlex->browse(palbum->id, 0, 100, onTrackFound);
    Serial.printf("Free memory: %d\n", esp_get_free_heap_size());
  }
  delete[] palbums;
  Serial.printf("Free memory: %d\n", esp_get_free_heap_size());

  Data.dumpDatabase();
}

void setup() {
  int n, t, count;

  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting");

  Output.begin();
  Serial.println("Output initialised");
  Output.addText(0, 0, "Starting...");

  if (LittleFS.begin(true)) {
    Serial.println("LittleFS initialised");
  } else {
    Serial.println("Error initialising LittleFS");
  }
  Serial.printf("Total bytes in file system: %d\n", LittleFS.totalBytes());

  Data.begin();
  Serial.println("Database initialised");

  encoder.begin();
  Serial.println("Encoder initialised");

  SPI.begin();
  Serial.println("SPI initialised");

  volumeBegin();
  Serial.println("Volume started");

  playerBegin();
  Serial.println("Player initialisation started");

  playerWait();

  //Player.playFile("/sample.mp3");

  wifiBegin();
  Serial.println("WiFi initialisation started");

  wifiWait();
  Player.play("http://192.168.68.106:32469/object/4cb63d3779b86d9d6e25/file.flac");

  /*Serial.println("Finding servers");
  DLNA.findServers(onServerFound);
  Serial.printf("Free memory: %d\n", esp_get_free_heap_size());
  Serial.printf("Plex server name: %s\n\n", pPlex->name);*/

  // refreshData();
  Albums.activate();
  // Data.dumpDatabase();
}

void loop() {
  int direction;

  encoderDebouncer.loop();

  if (direction = encoder.getDirection()) {
    EventManager.queueEvent(new EncoderEvent(direction));
  }

  EventManager.processEvents();

  delay(10);
}