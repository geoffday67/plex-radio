#include "VS1053-gd.h"

#include <Arduino.h>

#define SCI_READ 0x03
#define SCI_WRITE 0x02

#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_CLOCKF 0x03
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_AIADDR 0x0A
#define SCI_AICTRL0 0x0C
#define SCI_AICTRL1 0x0D
#define SCI_AICTRL2 0x0E
#define SCI_AICTRL3 0x0F

#define SM_CANCEL 0x0008
#define SM_TESTS 0x0020

void VS1053_gd::init() {
  // Chip selects are active low, so make them both high initially.
  pinMode(xcs, OUTPUT);
  digitalWrite(xcs, HIGH);
  pinMode(xdcs, OUTPUT);
  digitalWrite(xdcs, HIGH);

  pinMode(reset, OUTPUT);
  digitalWrite(reset, HIGH);

  // DREQ high indicates that at least 32 bytes of data can be transferred, or a
  // command can be sent.
  pinMode(dreq, INPUT);
}

void VS1053_gd::begin() {
  //hardwareReset();

  spi.begin();
  spi.setBitOrder(MSBFIRST);
  spi.setDataMode(SPI_MODE0);
  spi.setFrequency(1000000);
  writeSCI(SCI_CLOCKF, 6 << 12);
  spi.setFrequency(4000000);
  writeSCI(SCI_AUDATA, 44101);
}

void VS1053_gd::waitReady() {
  while (digitalRead(dreq) == 0) {
    yield();
  }
}

void VS1053_gd::hardwareReset() {
  digitalWrite(reset, LOW);
  delay(100);
  digitalWrite(reset, HIGH);
  delay(100);
  //waitReady();
}

int VS1053_gd::getVersion() { return (readSCI(SCI_STATUS) & 0xF0) >> 4; }

uint16_t VS1053_gd::getParametersVersion() {
  writeSCI(SCI_WRAMADDR, 0x1E02);
  return readSCI(SCI_WRAM);
}

uint8_t VS1053_gd::getFillByte() {
  writeSCI(SCI_WRAMADDR, 0x1E06);
  return readSCI(SCI_WRAM) & 0xFF;
}

uint16_t VS1053_gd::getLastSDI() {
  writeSCI(SCI_WRAMADDR, 0xC011);
  return readSCI(SCI_WRAM);
}

uint16_t VS1053_gd::getMode() { return readSCI(SCI_MODE); }

void VS1053_gd::sciSineTest() {
  writeSCI(SCI_AUDATA, 0xAC45);
  writeSCI(SCI_AICTRL0, 0x05C8);
  writeSCI(SCI_AICTRL1, 0x05C8);
  writeSCI(SCI_AIADDR, 0x4020);
}

void VS1053_gd::sdiSineTest() {
  /*
  Hardware reset.
  Set SM_TESTS.
  Send test command to SDI bus.
  0x53 0xEF 0x6E n 0 0 0 0 is sine command n = 11100011 = 0xE3
  */
  uint16_t mode = readSCI(SCI_MODE);
  writeSCI(SCI_MODE, mode | SM_TESTS);
  uint8_t data[] = {0x53, 0xEF, 0x6E, 0x03, 0x00, 0x00, 0x00, 0x00};
  writeSDI(data, 8);
}

void VS1053_gd::setVolume(int volume) {}

void VS1053_gd::startSCI() {
  digitalWrite(xcs, LOW);
  while (digitalRead(dreq) == 0);
}

void VS1053_gd::stopSCI() { digitalWrite(xcs, HIGH); }

uint16_t VS1053_gd::readSCI(uint8_t address) {
  digitalWrite(xcs, LOW);
  spi.write(SCI_READ);
  spi.write(address);
  uint16_t result = spi.transfer16(0);
  while (digitalRead(dreq) == 0);
  digitalWrite(xcs, HIGH);
  return result;
}

void VS1053_gd::writeSCI(uint8_t address, uint16_t value) {
  digitalWrite(xcs, LOW);
  spi.write(SCI_WRITE);
  spi.write(address);
  spi.write16(value);
  while (digitalRead(dreq) == 0);
  digitalWrite(xcs, HIGH);
}

void VS1053_gd::writeSDI(uint8_t *pdata, int count) {
  digitalWrite(xdcs, LOW);
  spi.writeBytes(pdata, count);
  while (digitalRead(dreq) == 0);
  digitalWrite(xdcs, HIGH);
}

void VS1053_gd::sendFile(File &file) {
  uint8_t buffer[32], fillByte;
  uint16_t mode;
  int n, m, count;

  digitalWrite(xdcs, LOW);
  count = file.readBytes((char*) buffer, 32);
  while (count > 0) {
    while (digitalRead(dreq) == 0);
    spi.writeBytes(buffer, count);
    count = file.readBytes((char*) buffer, 32);
  }
  Serial.println("File data sent");
  digitalWrite(xdcs, HIGH);

  fillByte = getFillByte();
  Serial.printf("Using fill byte 0x%02X\n", fillByte);
  memset(buffer, fillByte, 32);

  digitalWrite(xdcs, LOW);
  for(n = 0; n < 65; n++) {
    while (digitalRead(dreq) == 0);
    spi.writeBytes(buffer, 32);
  }
  digitalWrite(xdcs, HIGH);
  Serial.println("Large fill bytes sent");

  mode = readSCI(SCI_MODE);
  writeSCI(SCI_MODE, mode | SM_CANCEL);
  Serial.println("SM_CANCEL set");

  do {
    while (digitalRead(dreq) == 0);
    digitalWrite(xdcs, LOW);
    spi.writeBytes(buffer, 32);
    digitalWrite(xdcs, HIGH);
    //Serial.println("Small fill bytes sent");

    mode = readSCI(SCI_MODE);
    Serial.printf("Mode = 0x%04X\n", mode);
  } while(mode & SM_CANCEL);

  /*
  10.5.1 Playing a Whole File
This is the default playback mode.
10 OPERATION
1. Send an audio file to VS1053b.
2. Read extra parameter value endFillByte (Chapter 10.11).
3. Send at least 2052 bytes of endFillByte[7:0].
4. Set SCI_MODE bit SM_CANCEL.
5. Send at least 32 bytes of endFillByte[7:0].
6. Read SCI_MODE. If SM_CANCEL is still set, go to 5. If SM_CANCEL hasn’t cleared
after sending 2048 bytes, do a software reset (this should be extremely rare).
7. The song has now been successfully sent. HDAT0 and HDAT1 should now both contain
0 to indicate that no format is being decoded. Return to 1.
  */

}
