#include <Arduino.h>
#include <Debouncer.h>
#include <SPIFFS.h>
#include <driver/adc.h>
#include <driver/spi_master.h>
#include <esp32_wifi/wifi.h>
#include <freertos/event_groups.h>

#include <regex>

#include "buttons/play.h"
#include "buttons/stop.h"
#include "constants.h"
#include "data/data.h"
#include "dlna/dlna.h"
#include "esp_log.h"
#include "pins.h"
#include "player.h"
#include "rotary.h"
#include "screens/Albums.h"
#include "screens/Menu.h"
#include "screens/Output.h"
#include "utils.h"

#define VSPI_MOSI 23
#define VSPI_MISO 19
#define VSPI_CLK 18

#define ENCODER_CLK 39
#define ENCODER_DT 36
#define ENCODER_BUTTON 34

#define BACK_BUTTON 33

static const char *TAG = "MAIN";

EventGroupHandle_t plexRadioGroup;
ESP32Wifi network;
Rotary encoder(ENCODER_CLK, ENCODER_DT, ENCODER_BUTTON);

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
  EventManager.queueEvent(new PlayEvent(value));
}

Debouncer *pPlayDebouncer;

int getStopValue() {
  return digitalRead(STOP_BUTTON) == 0;
}
void stopChanged(int value) {
  if (value) {
    xEventGroupSetBits(plexRadioGroup, STOP_BIT);
  }
}

Debouncer *pStopDebouncer;

void restart() {
  network.stop();
  Data.close();
  delay(1000);
  ESP.restart();
}

void wifiTask(void *pparams) {
  EventBits_t bits;

  bits = xEventGroupWaitBits(plexRadioGroup, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(10000));
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    PlayButton::setState(Button::State::Off);
  } else {
    PlayButton::setState(Button::State::On);
  }
  vTaskDelete(NULL);
}

void wifiBegin() {
  startTask(wifiTask, "wifi");
  network.start(plexRadioGroup, WIFI_CONNECTED_BIT);
}

bool wifiWait() {
  EventBits_t bits;
  bits = xEventGroupWaitBits(plexRadioGroup, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(10000));
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    return false;
  }

  return true;
}

#include "ring.h"

void check(char *message, int expected, int actual) {
  ESP_LOGI(TAG, "%s, should be %d, is %d", message, expected, actual);
  if (expected != actual) {
    ESP_LOGE(TAG, "!! FAIL !!");
  }
}

void ringTest() {
  RingBuffer ring(5, plexRadioGroup);
  EventBits_t threshold = 0x0001;
  EventBits_t room = 0x0002;
  uint8_t data[] = {1, 2, 3, 4, 5}, buffer[5];

  ring.setThreshold(3, threshold);
  ring.setRoom(2, room);

  check("Space", 5, ring.room());
  check("Available", 0, ring.available());
  check("Threshold flag", 0, xEventGroupGetBits(plexRadioGroup) & threshold);
  check("Room flag", room, xEventGroupGetBits(plexRadioGroup) & room);

  ring.put(data, 3);
  check("Space", 2, ring.room());
  check("Available", 3, ring.available());
  check("Threshold flag", threshold, xEventGroupGetBits(plexRadioGroup) & threshold);
  check("Room flag", room, xEventGroupGetBits(plexRadioGroup) & room);

  ring.get(buffer, 3);
  check("Fetch 1", 1, buffer[0]);
  check("Fetch 2", 2, buffer[1]);
  check("Fetch 3", 3, buffer[2]);
  check("Space", 5, ring.room());
  check("Available", 0, ring.available());
  check("Threshold flag", 0, xEventGroupGetBits(plexRadioGroup) & threshold);
  check("Room flag", room, xEventGroupGetBits(plexRadioGroup) & room);

  ring.put(data, 5);
  check("Space", 0, ring.room());
  check("Available", 5, ring.available());
  check("Threshold flag", threshold, xEventGroupGetBits(plexRadioGroup) & threshold);
  check("Room flag", 0, xEventGroupGetBits(plexRadioGroup) & room);

  ring.get(buffer, 5);
  check("Fetch 1", 1, buffer[0]);
  check("Fetch 5", 5, buffer[4]);
  check("Space", 5, ring.room());
  check("Available", 0, ring.available());
  check("Threshold flag", 0, xEventGroupGetBits(plexRadioGroup) & threshold);
  check("Room flag", room, xEventGroupGetBits(plexRadioGroup) & room);

  ring.put(data, 2);
  check("Space", 3, ring.room());
  check("Available", 2, ring.available());
  check("Threshold flag", 0, xEventGroupGetBits(plexRadioGroup) & threshold);
  check("Room flag", room, xEventGroupGetBits(plexRadioGroup) & room);

  ring.get(buffer, 2);
  check("Fetch 1", 1, buffer[0]);
  check("Fetch 2", 2, buffer[1]);
  check("Space", 5, ring.room());
  check("Available", 0, ring.available());
  check("Threshold flag", 0, xEventGroupGetBits(plexRadioGroup) & threshold);
  check("Room flag", room, xEventGroupGetBits(plexRadioGroup) & room);
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
  Serial.println("Event groups created");

  wifiBegin();
  Serial.println("WiFi initialisation started");

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
  pPlayDebouncer = new Debouncer(getPlayValue, playChanged, 100);
  PlayButton::begin();
  PlayButton::setState(Button::State::Flashing);  // Flash while we connect WiFi.
  Serial.println("Play button initialised");

  pinMode(STOP_BUTTON, INPUT);
  pStopDebouncer = new Debouncer(getStopValue, stopChanged, 100);
  StopButton::begin();
  StopButton::setState(Button::State::On);
  Serial.println("Stop button initialised");

  memset(&spi_config, 0, sizeof spi_config);
  spi_config.mosi_io_num = VSPI_MOSI;
  spi_config.miso_io_num = VSPI_MISO;
  spi_config.sclk_io_num = VSPI_CLK;
  spi_bus_initialize(SPI3_HOST, &spi_config, SPI_DMA_DISABLED);
  Serial.println("SPI initialised");

  Player::begin();
  // Player::setVolume(80);
  Serial.println("Player initialised");

  /*Track test;
  strcpy(test.id, "123");
  strcpy(test.album, "Test album");
  strcpy(test.title, "Test track");
  // strcpy(test.resource, "http://192.168.68.106:32469/object/8759b6af55861818f5c0/file.flac");
  // strcpy(test.resource, "http://192.168.68.106:32469/object/f647173920a634673f22/file.flac");
  // strcpy(test.resource, "http://192.168.68.106:32469/object/3af33f13d6d2c1eb5cd5/file.flac");
  // strcpy(test.resource, "http://192.168.68.106:32469/object/2025ba9289137064ca17/file.mp3");
  Player::addToPlaylist(&test);*/

  /*wifiWait();
  Serial.println("Finding servers");
  DLNA.findServers(onServerFound);
  Serial.printf("Plex server name: %s\n", pPlex->name);
  refreshData();*/

  Menu.activate();
  // Albums.activate();
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
