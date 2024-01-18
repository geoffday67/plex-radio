#include <Arduino.h>
#include <Debouncer.h>
#include <SPIFFS.h>
#include <driver/adc.h>
#include <esp32_wifi/wifi.h>

#include <regex>

#include "constants.h"
#include "data/data.h"
#include "dlna/dlna.h"
#include "esp_log.h"
#include "player.h"
#include "rotary.h"
#include "screens/Albums.h"
#include "screens/Output.h"
#include "utils.h"

#define VSPI_MOSI 23
#define VSPI_MISO 19
#define VSPI_CLK 18

#define ENCODER_CLK 39
#define ENCODER_DT 36
#define ENCODER_BUTTON 34

#define BACK_BUTTON 33

#define PLAY_BUTTON 35
#define PLAY_LIGHT 32

#define STOP_BUTTON 26
#define STOP_LIGHT 27

static const char *TAG = "MAIN";

const EventBits_t WIFI_CONNECTED_BIT = 0x01;
EventGroupHandle_t plexRadioGroup;
ESP32Wifi network;
Rotary encoder(ENCODER_CLK, ENCODER_DT, ENCODER_BUTTON);

void showPlay(bool show) {
  if (show) {
    digitalWrite(PLAY_LIGHT, HIGH);
  } else {
    digitalWrite(PLAY_LIGHT, LOW);
  }
}

void showStop(bool show) {
  if (show) {
    digitalWrite(STOP_LIGHT, HIGH);
  } else {
    digitalWrite(STOP_LIGHT, LOW);
  }
}

int getEncoder() {
  return digitalRead(ENCODER_BUTTON);
}

void encoderChanged(int value) {
  EventManager.queueEvent(new SwitchEvent(value == 0));
}

Debouncer encoderDebouncer(getEncoder, encoderChanged, 100);

int getBackValue() {
  return digitalRead(BACK_BUTTON) == 0;
}

void backChanged(int value) {
  EventManager.queueEvent(new BackEvent(value));
}

Debouncer *pBackDebouncer;

int getPlayValue() {
  return digitalRead(PLAY_BUTTON) == 0;
}

void playChanged(int value) {
  Serial.printf("Play button %d\n", value);
}

Debouncer *pPlayDebouncer;

int getStopValue() {
  return digitalRead(STOP_BUTTON) == 0;
}

void stopChanged(int value) {
  Serial.printf("Stop button %d\n", value);
}

Debouncer *pStopDebouncer;

void wifiBegin() {
  network.start(plexRadioGroup, WIFI_CONNECTED_BIT);
}

bool wifiWait() {
  EventBits_t bits;
  bits = xEventGroupWaitBits(plexRadioGroup, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    return false;
  }

  return true;
}

DLNAServer *pPlex;

void onServerFound(DLNAServer *pserver) {
  // TODO Check that this is a Plex server, and maybe that we haven't seen it before.
  delete pPlex;
  pPlex = new DLNAServer(*pserver);
  ESP_LOGI(TAG, "Server found: %s", pserver->name);
}

void onAlbumFound(Object *pobject) {
  // Parse apart the album name to get the title and artist.
  // This is probably Plex-format specific.
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
  Serial.printf("Stored album: %s\n", palbum->title);

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
  Serial.printf("Stored track: %s\n", ptrack->title);
  delete ptrack;
}

void refreshData() {
  int n, count, offset, found;

  Serial.printf("Free memory: %d\n", esp_get_free_heap_size());

  Data.clearAll();
  Serial.println("Database cleared");

  // Fetch all the albums first and add them to the database.
  offset = 0;
  while (1) {
    // TODO Get this object ID dynnamically.
    // Request N album records, if none were found then we're done.
    Data.startTransaction();
    pPlex->browse("9b55ecd3e74c09febffe", offset, 10, "dc:title", &found, onAlbumFound);
    Data.endTransaction();
    break;  // TESTING
    if (found == 0) {
      break;
    }
    offset += found;
  }

  // Fetch the list of albums from the database and get the tracks for each.
  Album *palbums, *palbum;
  count = Data.getAlbums(&palbums);
  Serial.printf("%d albums fetched\n", count);
  for (n = 0; n < count; n++) {
    palbum = palbums + n;
    Serial.printf("Getting tracks for album %s BY %s\n", palbum->title, palbum->artist);
    strcpy(albumId, palbum->id);
    Data.beginStoreTracks();
    pPlex->browse(palbum->id, 0, 100, "res", NULL, onTrackFound);
    Data.endStoreTracks();
  }
  delete[] palbums;

  Data.dumpDatabase();

  Serial.printf("Free memory: %d\n", esp_get_free_heap_size());
}

void setup() {
  spi_bus_config_t spi_config;

  Serial.begin(230400);
  Serial.println();
  Serial.println("Starting");

  Output.begin();
  Serial.println("Output initialised");
  Output.addText(0, 0, "Starting...");

  plexRadioGroup = xEventGroupCreate();
  esp_event_loop_create_default();
  Serial.println("Event group created");

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
  pBackDebouncer = new Debouncer(getBackValue, backChanged, 100);
  Serial.println("Back button initialised");

  pinMode(PLAY_BUTTON, INPUT);
  pinMode(PLAY_LIGHT, OUTPUT);
  showPlay(true);
  pPlayDebouncer = new Debouncer(getPlayValue, playChanged, 100);
  Serial.println("Play button initialised");

  pinMode(STOP_BUTTON, INPUT);
  pinMode(STOP_LIGHT, OUTPUT);
  showStop(true);
  pStopDebouncer = new Debouncer(getStopValue, stopChanged, 100);
  Serial.println("Stop button initialised");

  memset(&spi_config, 0, sizeof spi_config);
  spi_config.mosi_io_num = VSPI_MOSI;
  spi_config.miso_io_num = VSPI_MISO;
  spi_config.sclk_io_num = VSPI_CLK;
  spi_bus_initialize(SPI3_HOST, &spi_config, SPI_DMA_DISABLED);
  Serial.println("SPI initialised");

  Player.begin();
  Serial.println("Player initialised");

  wifiBegin();
  Serial.println("WiFi initialisation started");

  /*wifiWait();
  Track test;
  strcpy(test.id, "123");
  strcpy(test.album, "Test album");
  strcpy(test.title, "Test track");
  // strcpy(test.resource, "http://192.168.68.106:32469/object/8759b6af55861818f5c0/file.flac");
  strcpy(test.resource, "http://192.168.68.106:32469/object/f647173920a634673f22/file.flac");
  // strcpy(test.resource, "http://192.168.68.106:32469/object/3af33f13d6d2c1eb5cd5/file.flac");
  Player.addToPlaylist(&test);
  return;*/

  /*Serial.println("Finding servers");
  DLNA.findServers(onServerFound);
  Serial.printf("Plex server name: %s\n", pPlex->name);
  refreshData();
  return;*/

  Albums.activate();
}

void loop() {
  int direction;

  encoderDebouncer.loop();
  pBackDebouncer->loop();
  pPlayDebouncer->loop();
  pStopDebouncer->loop();

  if (direction = encoder.getDirection()) {
    EventManager.queueEvent(new EncoderEvent(direction));
  }

  EventManager.processEvents();

  taskYIELD();
}
