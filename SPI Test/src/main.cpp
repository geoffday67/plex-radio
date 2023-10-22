#include <Arduino.h>

#include "driver/spi_master.h"

void setup() {
  esp_err_t e;
  spi_bus_config_t config;
  spi_device_interface_config_t dev_config;
  spi_device_handle_t handle;
  spi_transaction_t transaction;
  byte received[32];

  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting");

  // Setup SPI bus, send command to get chip version, get response.
  // Read register 0x01, bits 7 - 4 as the version (register is 16 bits total).
  // MOSI = 23, MISO = 19 => SPI3.
  memset(&config, 0, sizeof config);
  config.mosi_io_num = -1;
  config.miso_io_num = -1;
  config.sclk_io_num = -1;
  e = spi_bus_initialize(SPI3_HOST, &config, SPI_DMA_DISABLED);
  Serial.printf("Bus initialise returned %d\n", e);

  memset(&dev_config, 0, sizeof dev_config);
  dev_config.command_bits = 8;
  dev_config.address_bits = 8;
  dev_config.mode = 0;
  dev_config.clock_speed_hz = 100000;
  dev_config.spics_io_num = 13;
  dev_config.queue_size = 1;
  e = spi_bus_add_device(SPI3_HOST, &dev_config, &handle);
  Serial.printf("Add device returned %d\n", e);

  memset(&transaction, 0, sizeof transaction);
  transaction.cmd = 0x03;
  transaction.addr = 0x01;
  transaction.length = 16;
  transaction.rxlength = 16;
  transaction.rx_buffer = received;
  e = spi_device_transmit(handle, &transaction);
  Serial.printf("Transmit returned %d\n", e);



  e = spi_bus_remove_device(handle);
  Serial.printf("Remove device returned %d\n", e);

  spi_bus_free(SPI3_HOST);
  Serial.printf("Bus free returned %d\n", e);
}

void loop() {
}