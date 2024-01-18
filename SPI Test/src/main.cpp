#include <Arduino.h>
#include <SPIFFS.h>
#include <esp32_wifi/wifi.h>

#include "driver/spi_master.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "vs1053b.h"

#define VS1053_CS 21
#define VS1053_DCS 22
#define VS1053_RESET 4
#define VS1053_DREQ 15

#define VSPI_MOSI 23
#define VSPI_MISO 19
#define VSPI_CLK 18

VS1053b vs1053b(VS1053_CS, VS1053_DCS, VS1053_DREQ, VS1053_RESET);
const EventBits_t WIFI_CONNECTED_BIT = 0x01;
EventGroupHandle_t plexRadioGroup;
ESP32Wifi network;

void listFiles() {
  File root = SPIFFS.open("/");

  File file = root.openNextFile();

  while (file) {
    Serial.print("File: ");
    Serial.println(file.name());

    file = root.openNextFile();
  }
}

esp_err_t httpEventHandler(esp_http_client_event_t *evt) {
  return ESP_OK;
}

void startHttp() {
  esp_http_client_handle_t httpClient;
  esp_http_client_config_t httpConfig;
  int length, n;

  memset(&httpConfig, 0, sizeof httpConfig);
  httpConfig.url = "http://eo1thmnievwbew4.m.pipedream.net";
  httpConfig.user_agent = "ESP32";
  httpConfig.method = HTTP_METHOD_GET;
  httpConfig.event_handler = httpEventHandler;
  httpClient = esp_http_client_init(&httpConfig);

  esp_http_client_open(httpClient, 0);
  length = esp_http_client_fetch_headers(httpClient);
  Serial.printf("Status code %d\n", esp_http_client_get_status_code(httpClient));
  Serial.printf("Content length %d\n", length);
  char *presponse = new char[length + 1];
  n = esp_http_client_read(httpClient, presponse, length);
  Serial.printf("%d bytes read\n", n);
  presponse[length] = 0;
  Serial.print(presponse);
  esp_http_client_close(httpClient);
  esp_http_client_cleanup(httpClient);
}

void setup() {
  int n;
  spi_bus_config_t config;
  spi_device_interface_config_t dev_config;
  spi_transaction_t transaction;
  uint16_t value;
  wifi_init_config_t wifiInitConfig;

  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting");

  plexRadioGroup = xEventGroupCreate();
  esp_event_loop_create_default();
  network.start(plexRadioGroup, WIFI_CONNECTED_BIT);

  EventBits_t bits;
  bits = xEventGroupWaitBits(plexRadioGroup, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
  if ((bits & WIFI_CONNECTED_BIT) == 0) {
    return;
  }

  if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialised");
  } else {
    Serial.println("Error initialising SPIFFS");
  }
  Serial.printf("Total bytes in file system: %d\n", SPIFFS.totalBytes());
  listFiles();

  // Setup SPI bus.
  // Setting these pins to -1 possibly should make them default to IO_MUX direct usage but that doesn't seem to work.
  memset(&config, 0, sizeof config);
  config.mosi_io_num = VSPI_MOSI;
  config.miso_io_num = VSPI_MISO;
  config.sclk_io_num = VSPI_CLK;
  spi_bus_initialize(SPI3_HOST, &config, SPI_DMA_DISABLED);

  vs1053b.begin();
  Serial.printf("Version %d\n", vs1053b.getVersion());

  //vs1053b.setVolume(80);

  vs1053b.playUrl("http://192.168.68.106:32469/object/2025ba9289137064ca17/file.mp3");
  Serial.println("Song complete");
}

void loop() {
}