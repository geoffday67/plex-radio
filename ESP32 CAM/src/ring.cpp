#include "ring.h"

#include <memory.h>

#include "esp32-hal-log.h"

static const char *TAG = "RingBuffer";

RingBuffer::RingBuffer(int size, EventGroupHandle_t eventGroup) {
  pbuffer = new uint8_t[size];
  this->size = size;
  this->eventGroup = eventGroup;
  mutex = xSemaphoreCreateMutex();

  thresholdFlag = 0;
  roomFlag = 0;
  emptyFlag = 0;

  head = 0;
  tail = 0;
  used = 0;

  setFlags();
}

RingBuffer::~RingBuffer() {
  delete[] pbuffer;
}

void RingBuffer::setThreshold(int amount, EventBits_t flag) {
  this->thresholdAmount = amount;
  this->thresholdFlag = flag;
  setFlags();
}

void RingBuffer::setRoom(int amount, EventBits_t flag) {
  this->roomAmount = amount;
  this->roomFlag = flag;
  setFlags();
}

void RingBuffer::setEmpty(EventBits_t flag) {
  this->emptyFlag = flag;
  setFlags();
}

void RingBuffer::setFlags() {
  if (thresholdFlag != 0 && used >= thresholdAmount) {
    xEventGroupSetBits(eventGroup, thresholdFlag);
  } else {
    xEventGroupClearBits(eventGroup, thresholdFlag);
  }

  if (roomFlag != 0 && (size - used >= roomAmount)) {
    xEventGroupSetBits(eventGroup, roomFlag);
  } else {
    xEventGroupClearBits(eventGroup, roomFlag);
  }

  if (emptyFlag != 0 && used == 0) {
    xEventGroupSetBits(eventGroup, emptyFlag);
  } else {
    xEventGroupClearBits(eventGroup, emptyFlag);
  }
}

void RingBuffer::put(uint8_t *pdata, int count) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    // headSpace = direct room after "head".
    int headSpace = size - head;
    if (headSpace >= count) {
      // It all fits directly after "head", only a single copy is needed.
      memcpy(pbuffer + head, pdata, count);
      head += count;
      if (head == size) {
        head = 0;
      }
    } else {
      // It's bigger than the direct amount, we'll need two copies.
      int remainder = count - headSpace;
      memcpy(pbuffer + head, pdata, headSpace);
      memcpy(pbuffer, pdata + headSpace, remainder);
      head = remainder;
    }

    used += count;
    setFlags();
    xSemaphoreGive(mutex);
  }
}

void RingBuffer::get(uint8_t *pdata, int count) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    // tailSpace = the maximum amount directly after "tail" without wrapping.
    int tailSpace = size - tail;
    if (count <= tailSpace) {
      // We can satisfy the request directly without wrapping.
      memcpy(pdata, pbuffer + tail, count);
      tail += count;
      if (tail == size) {
        tail = 0;
      }
    } else {
      // We need two transfers.
      int remainder = count - tailSpace;
      memcpy(pdata, pbuffer + tail, tailSpace);
      memcpy(pdata + tailSpace, pbuffer, remainder);
      tail = remainder;
    }

    used -= count;
    setFlags();
    xSemaphoreGive(mutex);
  }
}

void RingBuffer::clear() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    head = 0;
    tail = 0;
    used = 0;
    setFlags();
    ESP_LOGI(TAG, "Ring buffer cleared");
    xSemaphoreGive(mutex);
  }
}