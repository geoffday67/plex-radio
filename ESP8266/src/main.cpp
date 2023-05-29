#include <Arduino.h>
#include <VS1053.h>
#include <FS.h>

#include "vs1053b-patches-flac.h"

#define VS1053_CS D1
#define VS1053_DCS D0
#define VS1053_DREQ D2
#define VS1053_RESET D3

#define VOLUME 100  // volume level 0-100

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

void setup() {
  int count;
  uint8_t buffer[1024];

  Serial.begin(115200);
  Serial.println();

  SPI.begin();
  Serial.println("SPI initialised");

  player.begin();
  Serial.println("Player initialised");

  if (player.isChipConnected()) {
    Serial.println("Chip connected");
  } else {
    Serial.println("Chip NOT connected");
  }

  player.switchToMp3Mode();
  player.loadUserCode(PATCH_FLAC, PATCH_FLAC_SIZE);
  player.setVolume(VOLUME);

  SPIFFS.begin();
  File file = SPIFFS.open("/hello.flac", "r");
  Serial.printf("File size = %d\n", file.size());

  //player.startSong();
  count = file.readBytes((char*)buffer, 1024);
  while (count > 0) {
    player.playChunk(buffer, count);
    count = file.readBytes((char*)buffer, 1024);
  }
  Serial.println("File data sent");
  //player.stopSong();

  Serial.println("Player finished");
}

void loop() {}
