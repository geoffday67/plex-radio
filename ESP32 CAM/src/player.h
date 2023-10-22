#pragma once

#include <HTTPClient.h>
#include <VS1053.h>

#include <queue>

#include "data/track.h"
#include "vs1053b-patches-flac.h"

#define VS1053_CS 21
#define VS1053_DCS 22
#define VS1053_DREQ 15
#define VS1053_RESET 4

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
  void clear();
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
  HTTPClient *pHttpClient;
  std::queue<Track> playlist;
  TaskHandle_t playlisthandle;
  TaskHandle_t readHandle;
  TaskHandle_t dataHandle;
  portMUX_TYPE playlistLock;
  Track currentTrack;
  bool isPlaying, stopPlay;

  static void dataTask(void *);
  static void readTask(void *);
  static void debugTask(void *);
  static void playlistTask(void *);

  void startPlay();

 public:
  classPlayer();
  ~classPlayer();
  bool begin();
  void setVolume(int);
  void addToPlaylist(Track *);
  void clearPlaylist();
  void resetPlaylist(Track *);
  void stop();
};

extern classPlayer Player;