#ifndef __VS1053_GD__
#define __VS1053_GD__

#include <SPI.h>
#include <FS.h>

class VS1053_gd {
 private:
  SPIClass &spi;
  int xcs, xdcs, dreq, reset;
  void init();
  void startSCI();
  void stopSCI();
  uint16_t readSCI(uint8_t address);
  void writeSCI(uint8_t address, uint16_t value);
  void writeSDI(uint8_t *pdata, int count);
  void hardwareReset();
  void waitReady();

 public:
  VS1053_gd(SPIClass &spi, int xcs, int xdcs, int dreq, int reset) : spi(spi), xcs(xcs), xdcs(xdcs), dreq(dreq), reset(reset) { init(); }
  void begin();
  int getVersion();
  uint16_t getParametersVersion();
  uint16_t getMode();
  byte getFillByte();
  uint16_t getLastSDI();
  void sendFile(File &file);
  void sdiSineTest();
  void sciSineTest();
  void setVolume(int);
};

#endif