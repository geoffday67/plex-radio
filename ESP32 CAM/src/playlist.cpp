#include "playlist.h"

#include <memory.h>

#include "esp32-hal-log.h"

static const char *TAG = "Playlist";

Playlist::Playlist(EventGroupHandle_t eventGroup) {
  this->eventGroup = eventGroup;
  mutex = xSemaphoreCreateMutex();
  readyFlag = 0;
  setFlags();
}

void Playlist::setFlags() {
  if (readyFlag != 0 && !playlist.empty()) {
    xEventGroupSetBits(eventGroup, readyFlag);
  } else {
    xEventGroupClearBits(eventGroup, readyFlag);
  }
}

void Playlist::setReady(EventBits_t flag) {
  this->readyFlag = flag;
  setFlags();
}

void Playlist::put(Track *ptrack, int count) {
  int n;

  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    for (n = 0; n < count; n++) {
      playlist.push(ptrack[n]);
      ESP_LOGD(TAG, "Track added to playlist: %s", ptrack[n].title);
    }
    setFlags();
    xSemaphoreGive(mutex);
  }
}

void Playlist::get(Track *ptrack) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    memcpy(ptrack, &(playlist.front()), sizeof(Track));
    playlist.pop();
    ESP_LOGD(TAG, "Track fetched from playlist: %s", ptrack->title);
    setFlags();
    xSemaphoreGive(mutex);
  }
}

void Playlist::clear() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    while (playlist.size() > 0) {
      playlist.pop();
    }
    ESP_LOGD(TAG, "Playlist cleared");
    setFlags();
    xSemaphoreGive(mutex);
  }
}