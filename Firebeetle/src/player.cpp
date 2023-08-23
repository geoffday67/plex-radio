#include "player.h"

#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "dlna/server.h"
#include "utils.h"

extern WiFiClient wifiClient;
extern DLNAServer *pPlex;

static const char *TAG = "Player";

classPlayer Player;

RingBuffer::RingBuffer(int size) {
  this->size = size;
  pData = new byte[size];
  head = 0;
  tail = 0;
}

RingBuffer::~RingBuffer() {
  delete[] pData;
}

int RingBuffer::space(void) {
  // Don't let the buffer become completely full or head will equal tail again and it will look like it's emopty.
  int space = tail - head - 1;
  if (space < 0) {
    space += size;
  }
  return space;
}

int RingBuffer::available() {
  int available = head - tail;
  if (available < 0) {
    available += size;
  }
  return available;
}

bool classPlayer::begin() {
  bool result = false;

  pVS1053 = new VS1053(VS1053_CS, VS1053_DCS, VS1053_DREQ);

  pVS1053->begin();

  /*if (pVS1053->isChipConnected()) {
    Serial.println("Chip connected");
  } else {
    Serial.println("Chip NOT connected");
  }*/

  pVS1053->switchToMp3Mode();
  pVS1053->loadUserCode(PATCH_FLAC, PATCH_FLAC_SIZE);
  pVS1053->setVolume(100);  // TESTING

  result = true;

exit:
  return result;
}

void classPlayer::setVolume(int volume) {
  pVS1053->setVolume(volume);
}

/*
Only send when DREQ asserted.
Check for endedness and rising/falling clock sync, page 17.
Be aware of minimum clock pulse widths, which are based on CLKI cycles, page 18.
Library SPI maybe has too much overhead - write our own for this part?
*/

void classPlayer::dataTask(void *pdata) {
  classPlayer *pPlayer = (classPlayer *)pdata;
  RingBuffer *pring = pPlayer->pBuffer;
  int count, chunk;

  while (true) {
    // "count" is the number of bytes available to send.
    count = MIN(pring->available(), 1000);
    if (count > 0) {
      if (pring->head > pring->tail) {
        pPlayer->pVS1053->playChunk(pring->pTail(), count);
      } else {
        chunk = MIN(pring->size - pring->tail, count);
        pPlayer->pVS1053->playChunk(pring->pTail(), chunk);
        chunk = count - chunk;
        if (chunk > 0) {
          pPlayer->pVS1053->playChunk(pring->pData, chunk);
        }
      }

      pring->tail += count;
      if (pring->tail > pring->size) {
        pring->tail -= pring->size;
      }
    } else {
      vTaskSuspend(NULL);
    }

    esp_task_wdt_reset();
  }
}

void classPlayer::play(char *purl) {
  HTTPClient http_client;
  int n, count, processed, space, available, chunk, size, buffer_size;
  byte b;
  unsigned long start;
  TaskHandle_t dataHandle;
  bool task_started = false;

  /*if (!pPlex) {
    return;
  }*/

  // Create a task on the second core to send data to the VS1053 via the ring buffer.
  buffer_size = 100000;  // esp_get_free_heap_size() - 10240;
  Serial.printf("Buffer size: %d\n", buffer_size);
  pBuffer = new RingBuffer(buffer_size);

  Serial.printf("Requesting track from %s\n", purl);
  http_client.setReuse(false);
  http_client.begin(wifiClient, purl);
  int code = http_client.GET();
  if (code != 200) {
    Serial.printf("Got code %d loading resource\n", code);
    goto exit;
  }

  size = http_client.getSize();
  Serial.printf("%ld bytes total\n", size);
  processed = 0;

  while (processed < size) {
    space = pBuffer->space();
    if (space == 0) {
      continue;
    }
    // Serial.printf("%d buffer bytes available\n", space);

    start = millis();
    if (wifiClient.available() == 0) {
      Serial.println("No data!");
    }
    while ((available = wifiClient.available()) == 0) {
      // Serial.println("Waiting for data");
      if (millis() - start > 1000) {
        ESP_LOGW(TAG, "Timeout waiting for resource response");
        goto exit;
      }
      delay(10);
    }
    // Serial.printf("%d response bytes available\n", available);

    // "count" is the total number of bytes we're going to add to the buffer.
    count = MIN(available, space);

    if (pBuffer->head >= pBuffer->tail) {
      // Write from the head to buffer end, then from buffer start to the tail if there's more to write.
      chunk = MIN(pBuffer->size - pBuffer->head, count);
      wifiClient.readBytes(pBuffer->pHead(), chunk);
      chunk = count - chunk;
      if (chunk > 0) {
        wifiClient.readBytes(pBuffer->pData, chunk);
      }
    } else {
      // Write everything at head.
      wifiClient.readBytes(pBuffer->pHead(), count);
    }

    // Bump the buffer head but only set it once as that's what triggers the sending process.
    if (pBuffer->head + count > pBuffer->size) {
      pBuffer->head += count - pBuffer->size;
    } else {
      pBuffer->head += count;
    }
    // Serial.printf("Head now %d, tail %d\n", pBuffer->head, pBuffer->tail);

    if (!task_started && pBuffer->head > pBuffer->size / 2) {
      xTaskCreatePinnedToCore(
          dataTask,    /* Function to implement the task */
          "Data task", /* Name of the task */
          10000,       /* Stack size in words */
          this,        /* Task input parameter */
          2,           /* Priority of the task */
          &dataHandle, /* Task handle. */
          1);          /* Core where the task should run */

      task_started = true;
      Serial.println("Data task created");
    }

    if (task_started) {
      vTaskResume(dataHandle);
    }

    // Bump the total amount processed of the response.
    processed += count;

    delay(1);
  }

  http_client.end();

  // TODO Wait for buffer to be empty then delete the task.

  Serial.println("Finished playing");

exit:
  return;
}

void classPlayer::playFile(char *pfilename) {
  int count;
  uint8_t buffer[1024];

  File file = LittleFS.open(pfilename, "r");
  Serial.printf("File size = %d\n", file.size());

  count = file.readBytes((char *)buffer, 1024);
  while (count > 0) {
    pVS1053->playChunk(buffer, count);
    count = file.readBytes((char *)buffer, 1024);
  }

  Serial.println("Player finished");
}