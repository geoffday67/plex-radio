#pragma once

#include "driver/spi_master.h"

class VS1053b {
 private:
  static char TAG[];

  static const uint8_t COMMAND_WRITE = 0x02;
  static const uint8_t COMMAND_READ = 0x03;

  static const uint8_t REGISTER_MODE = 0x00;
  static const uint8_t REGISTER_STATUS = 0x01;
  static const uint8_t REGISTER_CLOCKF = 0x03;
  static const uint8_t REGISTER_WRAM = 0x06;
  static const uint8_t REGISTER_WRAM_ADDR = 0x07;

  static const uint16_t SM_RESET = 0x0004;
  static const uint16_t SM_CANCEL = 0x0008;
  static const uint16_t SM_SDINEW = 0x0800;

  static const uint16_t GPIO_DDR_RW_ADDR = 0xC017;
  static const uint16_t GPIO_ODATA_RW_ADDR = 0xC019;
  static const uint16_t END_FILL_BYTE_ADDR = 0x1E06;

  int csPin, dcsPin, dreqPin, resetPin;
  spi_device_handle_t handleCommand, handleData;

  void writeRegister(uint8_t target, uint16_t value);
  uint16_t readRegister(uint8_t target);

  void writeWram(uint16_t target, uint16_t value);
  uint16_t readWram(uint16_t target);

  void waitReady();
  void hardwareReset();
  void softwareReset();
  void switchToMP3();
  uint8_t getEndFillByte();
  void finaliseSong();

 public:
  VS1053b(int cs, int dcs, int dreq, int reset);
  void begin();
  int getVersion();
  void playFile();
  void playUrl(char *purl);
  void playChunk(uint8_t *pdata, int size);
};