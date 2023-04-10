#ifndef __VS1053__
#define __VS1053__

#include <SPI.h>

class VS1053 {
 private:
  SPIClass &spi;
  int xcs;
  void init();

 public:
  VS1053(SPIClass &spi, int xcs) : spi(spi), xcs(xcs) { init(); }
  int getVersion();
};

#endif