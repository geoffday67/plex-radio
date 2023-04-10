#include <Arduino.h>
#include <SPI.h>

#define XCS   13
#define READ  0x03
#define WRITE 0x02

SPIClass audio(VSPI);

void printBinary(int value) {
  int n;

  for (n = 0; n < 16; n++, value <<= 1) {
    if (value & 0x8000) {
      Serial.print("1");
    } else {
      Serial.print("0");
    }
    if (n % 4 == 3) {
      Serial.print(" ");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting.");
  
  pinMode(XCS, OUTPUT);
  digitalWrite(XCS, HIGH);
  audio.begin();
  audio.setBitOrder(SPI_MSBFIRST);
  audio.setDataMode(SPI_MODE0);
  audio.setFrequency(1000000);
  Serial.printf("Audio SPI initialised using CLK %d, MISO %d, MOSI %d, SS %d\n", SCK, MISO, MOSI, SS);

  delay(2000);

  digitalWrite(XCS, LOW);
  audio.write(READ);
  audio.write(0x01);
  u_int16_t result = audio.transfer16(0xABCD);
  digitalWrite(XCS, HIGH);
  printBinary(result);
  Serial.println();
}

void loop() {
}