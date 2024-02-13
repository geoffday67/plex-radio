#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

class RingBuffer {
 private:
  uint8_t *pbuffer;
  EventGroupHandle_t eventGroup;
  SemaphoreHandle_t mutex;

  int size, used, head, tail;
  int thresholdAmount, roomAmount;
  EventBits_t thresholdFlag, roomFlag, emptyFlag;

  void setFlags();

 public:
  RingBuffer(int size, EventGroupHandle_t eventGroup);
  ~RingBuffer();

  void setThreshold(int, EventBits_t);
  void setRoom(int, EventBits_t);
  void setEmpty(EventBits_t);

  void put(uint8_t *pdata, int count);
  void get(uint8_t *pdata, int count);
  void clear();

  int room() { return size - used; };
  int available() { return used; }
};