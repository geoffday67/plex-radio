#include <Arduino.h>
#include <Debouncer.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <driver/adc.h>
#include <esp_wifi.h>

#include <regex>

#include "constants.h"
#include "data/data.h"
#include "dlna/dlna.h"
#include "player.h"
#include "rotary.h"
#include "screens/Albums.h"
#include "screens/Output.h"
#include "utils.h"

#define ENCODER_CLK 34
#define ENCODER_DT 39
#define ENCODER_BUTTON 36
#define BACK_BUTTON 32
#define BACK_LIGHT 33

WiFiClient wifiClient;
volatile bool wifiConnected = false;
volatile bool playerReady = false;
Rotary encoder(ENCODER_CLK, ENCODER_DT, ENCODER_BUTTON);

int getEncoder() {
  return digitalRead(ENCODER_BUTTON);
}

void encoderChanged(int value) {
  EventManager.queueEvent(new SwitchEvent(value == 0));
}

Debouncer encoderDebouncer(getEncoder, encoderChanged, 100);

int getBackValue() {
  return digitalRead(BACK_BUTTON);
}

void backChanged(int value) {
  EventManager.queueEvent(new BackEvent(value == 0));
}

Debouncer backDebouncer(getBackValue, backChanged, 100);

void playerBeginTask(void *pdata) {
  if (Player.begin()) {
    playerReady = true;
    Serial.println("Player initialised");
  } else {
    Serial.println("Error initialising player");
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
  int count, n, max_rssi, network;

  // Wait for scan to be finished.
  while ((count = WiFi.scanComplete()) < 0) {
    vTaskDelay(10);
  }

  Serial.printf("%d networks found\n", count);
  for (n = 0; n < count; n++) {
    Serial.printf("%d: RSSI = %d, SSID = %s, BSSID = %s\n", n, WiFi.RSSI(n), WiFi.SSID(n).c_str(), WiFi.BSSIDstr(n).c_str());
  }

  max_rssi = -999;
  network = -1;
  for (n = 0; n < count; n++) {
    if (WiFi.RSSI(n) > max_rssi) {
      max_rssi = WiFi.RSSI(n);
      network = n;
    }
  }
  if (network == -1) {
    Serial.println("No usable network found");
    goto exit;
  }

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect();
  WiFi.hostname("Plex radio");
  esp_wifi_set_ps(WIFI_PS_NONE);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin("Wario", "mansion1", WiFi.channel(network), WiFi.BSSID(network));
    Serial.println("Connecting WiFi");

    start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - start > 5000) {
        Serial.println("Timed out connecting to access point");
        break;
      }
      delay(100);
    }
  }

  Serial.printf("Connected as %s to access point %s\n", WiFi.localIP().toString().c_str(), WiFi.BSSIDstr().c_str());
  wifiConnected = true;

exit:
  vTaskDelete(NULL);
}

void wifiBegin() {
  // Scan for networks we know and pick the strongest signal.
  WiFi.scanNetworks(true, false, false, 100, 0, "Wario");

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
  // TODO Add timeout
}

DLNAServer *pPlex;

void onServerFound(DLNAServer *pserver) {
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
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting");

  Output.begin();
  Serial.println("Output initialised");
  Output.addText(0, 0, "Starting...");

  if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialised");
  } else {
    Serial.println("Error initialising SPIFFS");
  }
  Serial.printf("Total bytes in file system: %d\n", SPIFFS.totalBytes());

  Serial.printf("Total bytes in PSRAM: %d\n", ESP.getPsramSize());

  Data.begin();
  Serial.println("Database initialised");

  pinMode(ENCODER_BUTTON, INPUT);
  encoder.begin();
  Serial.println("Encoder initialised");

  pinMode(BACK_BUTTON, INPUT_PULLUP);
  pinMode(BACK_LIGHT, OUTPUT);
  Serial.println("Buttons initialised");

  SPI.begin();
  Serial.println("SPI initialised");

  playerBegin();
  Serial.println("Player initialisation started");

  wifiBegin();
  Serial.println("WiFi initialisation started");

  /*Serial.println("Finding servers");
  DLNA.findServers(onServerFound);
  Serial.printf("Plex server name: %s\n\n", pPlex->name);*/

  //startTask(backTask, "back");

  Albums.activate();
  digitalWrite(BACK_LIGHT, HIGH);
  
  // refreshData();
  // Data.dumpDatabase();
}

void loop() {
  int direction;

  encoderDebouncer.loop();
  backDebouncer.loop();

  if (direction = encoder.getDirection()) {
    EventManager.queueEvent(new EncoderEvent(direction));
  }

  EventManager.processEvents();

  taskYIELD();
}