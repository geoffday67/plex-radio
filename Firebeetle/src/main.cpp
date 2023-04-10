#include <Arduino.h>
#include <SPI.h>
#include <SPIFFS.h>

#include "VS1053.h"

#define XCS_PIN 13

SPIClass vsSPI(VSPI);
VS1053 vs1053(vsSPI, XCS_PIN);

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

  SPIFFS.begin();
  File file = SPIFFS.open("/audio.mp3");
  Serial.printf("File size is %d\n", file.size());
  file.close();

  delay(2000);

  int version = vs1053.getVersion();
  Serial.printf("Board version %d\n", version);
}

void loop() {}