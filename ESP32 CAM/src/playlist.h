#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include <queue>

#include "data/track.h"

class Playlist {
 private:
  std::queue<Track> playlist;
  EventGroupHandle_t eventGroup;
  EventBits_t readyFlag;
  void setFlags();
  SemaphoreHandle_t mutex;

 public:
  Playlist(EventGroupHandle_t eventGroup);
  void setReady(EventBits_t);

  void put(Track *, int);
  void get(Track *);
  void clear();
};