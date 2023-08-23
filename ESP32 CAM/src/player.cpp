#include "player.h"

#include <SPIFFS.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "constants.h"
#include "utils.h"

extern WiFiClient wifiClient;
extern void playerWait(void);
extern void wifiWait(void);

static const char *TAG = "Player";

classPlayer Player;

RingBuffer::RingBuffer(int size) {
  this->size = size;
  pData = new byte[size];
  if (pData) {
    Serial.printf("%d bytes allocated\n", size);
  } else {
    Serial.printf("Error allocating %d bytes\n", size);
  }
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

classPlayer::classPlayer() {
  pVS1053 = 0;
  pBuffer = 0;
  pHttpClient = 0;
}

classPlayer::~classPlayer() {
  delete pVS1053;
  delete pBuffer;
  delete pHttpClient;
}

bool classPlayer::begin() {
  bool result = false;

  pVS1053 = new VS1053(VS1053_CS, VS1053_DCS, VS1053_DREQ);

  pVS1053->begin();

  if (pVS1053->isChipConnected()) {
    Serial.println("Chip connected");
  } else {
    Serial.println("Chip NOT connected");
  }

  pVS1053->switchToMp3Mode();
  pVS1053->loadUserCode(PATCH_FLAC, PATCH_FLAC_SIZE);
  pVS1053->setVolume(90);  // TESTING

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

// Send data from the ring buffer to the VS1053.
void classPlayer::dataTask(void *pdata) {
  classPlayer *pPlayer = (classPlayer *)pdata;
  RingBuffer *pring = pPlayer->pBuffer;
  int count, chunk;

  while (true) {
    count = MIN(pring->available(), 10000);
    if (count > 0) {
      // Serial.printf("Sending %d bytes\n", count);
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
      // vTaskSuspend(NULL);
    }
    taskYIELD();
  }
}

// Fetch data from the network and add it to the ring buffer.
void classPlayer::readTask(void *pdata) {
  classPlayer *pPlayer = (classPlayer *)pdata;
  RingBuffer *pring = pPlayer->pBuffer;
  int processed, size, count, space, available, chunk;
  unsigned long start;
  bool task_started = false;
  TaskHandle_t dataHandle;

  size = pPlayer->pHttpClient->getSize();
  Serial.printf("%ld bytes total\n", size);
  processed = 0;

  while (processed < size) {
    space = pPlayer->pBuffer->space();
    if (space == 0) {
      continue;
    }

    start = millis();
    while ((available = wifiClient.available()) == 0) {
      if (millis() - start > 5000) {
        Serial.println("Timeout waiting for resource response");
        goto exit;
      }
      taskYIELD();
    }

    // "count" is the total number of bytes we're going to add to the buffer.
    count = MIN(available, space);

    if (pPlayer->pBuffer->head >= pPlayer->pBuffer->tail) {
      // Write from the head to buffer end, then from buffer start to the tail if there's more to write.
      chunk = MIN(pPlayer->pBuffer->size - pPlayer->pBuffer->head, count);
      wifiClient.readBytes(pPlayer->pBuffer->pHead(), chunk);
      chunk = count - chunk;
      if (chunk > 0) {
        wifiClient.readBytes(pPlayer->pBuffer->pData, chunk);
      }
    } else {
      // Write everything at head.
      wifiClient.readBytes(pPlayer->pBuffer->pHead(), count);
    }

    // Bump the buffer head but only set it once as that's what triggers the sending process.
    if (pPlayer->pBuffer->head + count > pPlayer->pBuffer->size) {
      pPlayer->pBuffer->head += count - pPlayer->pBuffer->size;
    } else {
      pPlayer->pBuffer->head += count;
    }

    // if (!task_started && pPlayer->pBuffer->head > pPlayer->pBuffer->size / 5) {
    if (!task_started && pPlayer->pBuffer->available() > 100000) {
      xTaskCreatePinnedToCore(
          dataTask,
          "Data task",
          ARDUINO_STACK,
          pPlayer,
          ARDUINO_PRIORITY,
          &dataHandle,
          ARDUINO_CORE);

      task_started = true;
      Serial.println("Data task created");
    }

    /*if (task_started) {
      vTaskResume(dataHandle);
    }*/

    // Bump the total amount processed of the response.
    processed += count;

    taskYIELD();
  }

exit:
  pPlayer->pHttpClient->end();

  // TODO Wait for buffer to be empty then delete the task.
  /*if (task_started) {
    vTaskDelete(dataHandle);
  }*/
  vTaskDelete(NULL);

  Serial.println("Finished playing");
}

void classPlayer::debugTask(void *pdata) {
  while (1) {
    classPlayer *pplayer = (classPlayer *)pdata;
    RingBuffer *pbuffer = pplayer->pBuffer;
    Serial.println(pbuffer->available());
    vTaskDelay(100);
  }
}

void classPlayer::play(char *purl) {
  wifiWait();
  Serial.println("WiFi ready");
  playerWait();
  Serial.println("Player ready");

  pBuffer = new RingBuffer(3000000);

  Serial.printf("Requesting track from %s\n", purl);
  pHttpClient = new HTTPClient;
  pHttpClient->setReuse(false);
  pHttpClient->begin(wifiClient, purl);
  int code = pHttpClient->GET();
  if (code != 200) {
    Serial.printf("Got code %d loading resource\n", code);
    goto exit;
  }

  /*xTaskCreatePinnedToCore(
      dataTask,
      "Data task",
      ARDUINO_STACK,
      this,
      ARDUINO_PRIORITY,
      NULL,
      ARDUINO_CORE);*/

  xTaskCreatePinnedToCore(
      readTask,
      "Read task",
      ARDUINO_STACK,
      this,
      ARDUINO_PRIORITY,
      NULL,
      ARDUINO_CORE);

  /*xTaskCreatePinnedToCore(
      debugTask,
      "Debug task",
      ARDUINO_STACK,
      this,
      ARDUINO_PRIORITY,
      NULL,
      ARDUINO_CORE);*/

exit:
  return;
}