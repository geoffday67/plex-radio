#include "vs1053b.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp32-hal-log.h"
#include "esp_http_client.h"
#include "freertos/task.h"
#include "utils.h"

VS1053b *pInstance;

char VS1053b::TAG[] = "VS1053b";

VS1053b::VS1053b(int cs, int dcs, int dreq, int reset) {
  csPin = cs;
  dcsPin = dcs;
  dreqPin = dreq;
  resetPin = reset;
  eventGroup = xEventGroupCreate();
}

void VS1053b::DREQISR() {
  xEventGroupSetBitsFromISR(pInstance->eventGroup, 0x01, NULL);
}

void VS1053b::begin() {
  spi_device_interface_config_t dev_config;

  pInstance = this;

  // Configure pins.
  gpio_set_direction((gpio_num_t)dreqPin, GPIO_MODE_INPUT);

  gpio_set_direction((gpio_num_t)resetPin, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)resetPin, 1);

  gpio_set_direction((gpio_num_t)dcsPin, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)dcsPin, 1);

  gpio_set_direction((gpio_num_t)csPin, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)csPin, 1);

  ESP_LOGI(TAG, "Pins initialised");

  hardwareReset();
  ESP_LOGI(TAG, "Hardware reset");

  // Setup for interactions with the command stream; specify a slow SPI speed initially.
  memset(&dev_config, 0, sizeof dev_config);
  dev_config.flags = SPI_DEVICE_HALFDUPLEX;
  dev_config.command_bits = 8;
  dev_config.address_bits = 8;
  dev_config.clock_speed_hz = 100000;
  dev_config.spics_io_num = csPin;
  dev_config.queue_size = 1;
  spi_bus_add_device(SPI3_HOST, &dev_config, &handleCommand);

  switchToMP3();
  softwareReset();
  ESP_LOGI(TAG, "Switched to MP3 mode and software reset");

  // Set a clock multiplier then use a higher SPI speed.
  writeRegister(REGISTER_CLOCKF, 0x8800);
  vTaskDelay(10);

  spi_bus_remove_device(handleCommand);

  memset(&dev_config, 0, sizeof dev_config);
  dev_config.flags = SPI_DEVICE_HALFDUPLEX;
  dev_config.command_bits = 8;
  dev_config.address_bits = 8;
  dev_config.clock_speed_hz = 5000000;
  dev_config.spics_io_num = csPin;
  dev_config.queue_size = 1;
  spi_bus_add_device(SPI3_HOST, &dev_config, &handleCommand);

  // Setup for interactions with the data stream.
  memset(&dev_config, 0, sizeof dev_config);
  dev_config.flags = SPI_DEVICE_HALFDUPLEX;
  dev_config.clock_speed_hz = 5000000;
  dev_config.spics_io_num = dcsPin;
  dev_config.queue_size = 1;
  spi_bus_add_device(SPI3_HOST, &dev_config, &handleData);

  ESP_LOGI(TAG, "Data channel initialised");
}

void VS1053b::hardwareReset() {
  gpio_set_level((gpio_num_t)resetPin, 0);
  vTaskDelay(10);
  gpio_set_level((gpio_num_t)resetPin, 1);
  vTaskDelay(10);
}

void VS1053b::softwareReset() {
  writeRegister(REGISTER_MODE, SM_SDINEW | SM_RESET);
  vTaskDelay(10);
}

void VS1053b::waitReady() {
  // xEventGroupWaitBits(eventGroup, 0x01, pdTRUE, pdTRUE, portMAX_DELAY);
  while (gpio_get_level((gpio_num_t)dreqPin) == 0) {
    taskYIELD();
  }
}

uint16_t VS1053b::readRegister(uint8_t target) {
  spi_transaction_t transaction;
  uint16_t result;

  memset(&transaction, 0, sizeof transaction);
  transaction.flags = SPI_TRANS_USE_RXDATA;
  transaction.cmd = COMMAND_READ;
  transaction.addr = target;
  transaction.length = 16;
  transaction.rxlength = 16;
  waitReady();
  spi_device_transmit(handleCommand, &transaction);
  result = (transaction.rx_data[0] * 256) + transaction.rx_data[1];
  ESP_LOGD(TAG, "Reading 0x%04x from register 0x%02x", result, target);
  return result;
}

void VS1053b::writeRegister(uint8_t target, uint16_t value) {
  spi_transaction_t transaction;

  memset(&transaction, 0, sizeof transaction);
  transaction.flags = SPI_TRANS_USE_TXDATA;
  transaction.cmd = COMMAND_WRITE;
  transaction.addr = target;
  transaction.length = 16;
  transaction.tx_data[0] = value / 256;
  transaction.tx_data[1] = value % 256;
  waitReady();
  spi_device_transmit(handleCommand, &transaction);
  ESP_LOGD(TAG, "Writing 0x%04x to register 0x%02x", value, target);
}

uint16_t VS1053b::readWram(uint16_t target) {
  writeRegister(REGISTER_WRAM_ADDR, target);
  return readRegister(REGISTER_WRAM);
}

void VS1053b::writeWram(uint16_t target, uint16_t value) {
  writeRegister(REGISTER_WRAM_ADDR, target);
  writeRegister(REGISTER_WRAM, value);
}

void VS1053b::switchToMP3() {
  writeWram(GPIO_DDR_RW_ADDR, 0x0003);
  writeWram(GPIO_ODATA_RW_ADDR, 0x0000);
}

int VS1053b::getVersion() {
  uint16_t value = readRegister(REGISTER_STATUS);
  return (value & 0x00F0) >> 4;
}

void VS1053b::setVolume(int volume) {
  uint16_t value;

  if (volume == 0) {
    value = 0xFEFE;
  } else {
    value = 255 - (volume * 255 / 100);
    value |= value << 8;
  }
  writeRegister(REGISTER_VOL, value);
}

uint8_t VS1053b::getEndFillByte() {
  uint16_t value = readWram(END_FILL_BYTE_ADDR);
  return (value & 0xFF);
}

void VS1053b::finaliseSong() {
  uint8_t buffer[32], fill;
  int n;
  spi_transaction_t transaction;

  fill = getEndFillByte();
  ESP_LOGD(TAG, "Using 0x%02X end fill byte", fill);

  memset(buffer, fill, 32);
  for (n = 0; n < 65; n++) {
    memset(&transaction, 0, sizeof transaction);
    transaction.tx_buffer = buffer;
    transaction.length = 256;

    waitReady();
    spi_device_transmit(handleData, &transaction);
  }

  writeRegister(REGISTER_MODE, SM_CANCEL | SM_SDINEW);

  while (1) {
    memset(&transaction, 0, sizeof transaction);
    transaction.tx_buffer = buffer;
    transaction.length = 256;
    waitReady();
    spi_device_transmit(handleData, &transaction);

    // TODO Reset device if too long waiting for cancel to clear.

    if ((readRegister(REGISTER_MODE) & SM_CANCEL) == 0) {
      break;
    }
  }
}

void VS1053b::sendChunk(uint8_t *pdata, int size) {
  spi_transaction_t transaction;

  memset(&transaction, 0, sizeof transaction);
  transaction.tx_buffer = pdata;
  transaction.length = size * 8;
  waitReady();
  spi_device_transmit(handleData, &transaction);
}