#pragma once

#include <VS1053.h>

#include "vs1053b-patches-flac.h"

#define VS1053_CS 13
#define VS1053_DCS 25
#define VS1053_DREQ 26

class RingBuffer {
 public:
  RingBuffer(int);
  ~RingBuffer();
  byte *pData;
  int size, head, tail;

  int space();
  int available();
  byte *pHead() { return pData + head; }
  byte *pTail() { return pData + tail; }
};

class Chunk {
 private:
  int count;
  byte *pData;

  friend class classPlayer;
};

class classPlayer {
 private:
  VS1053 *pVS1053;
  RingBuffer *pBuffer;

  static void dataTask(void *);

 public:
  bool begin();
  void play(char *);
  void playFile(char *);
  void setVolume(int);
};

extern classPlayer Player;