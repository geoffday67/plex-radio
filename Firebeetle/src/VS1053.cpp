#include "VS1053.h"

#include <Arduino.h>

#define SCI_READ 0x03
#define SCI_WRITE 0x02

#define SCI_STATUS 0x01

void VS1053::init() {
  pinMode(xcs, OUTPUT);
  digitalWrite(xcs, HIGH);
  spi.begin();
  spi.setBitOrder(SPI_MSBFIRST);
  spi.setDataMode(SPI_MODE0);
  spi.setFrequency(1000000);
}

int VS1053::getVersion() {
  digitalWrite(xcs, LOW);
  spi.write(SCI_READ);
  spi.write(SCI_STATUS);
  u_int16_t result = spi.transfer16(0);
  digitalWrite(xcs, HIGH);
  return (result & 0xF0) >> 4;
}