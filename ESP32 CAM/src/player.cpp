#include "player.h"

#include <esp_task_wdt.h>

#include "constants.h"
#include "esp32-hal-log.h"
#include "utils.h"

extern void wifiWait(void);

static const char *TAG = "Player";

classPlayer Player;

RingBuffer::RingBuffer(int size) {
  this->size = size;
  pData = new uint8_t[size];
  if (pData) {
    ESP_LOGI(TAG, "%d bytes allocated", size);
  } else {
    ESP_LOGE(TAG, "Error allocating %d bytes", size);
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

void RingBuffer::clear() {
  tail = head;
}

classPlayer::classPlayer() {
  pVS1053b = 0;
  pBuffer = 0;
  readHandle = 0;
}

classPlayer::~classPlayer() {
  delete pVS1053b;
  delete pBuffer;
}

/*
Interrupt-driven sending.
Interrupt happens when DREQ is high but don't want continuous interrupts.
  Enable this interrupt only when playing a song?
During ISR:
  Check there's at least 32 bytes of data to send.
  We know that DREQ's high so send immediately.
  Only send 32 bytes then return so as to keep the ISR quick.
*/

bool classPlayer::begin() {
  bool result = false;

  pVS1053b = new VS1053b(VS1053_CS, VS1053_DCS, VS1053_DREQ, VS1053_RESET);
  pVS1053b->begin();

  pBuffer = new RingBuffer(3000000);
  dataHandle = startTask(dataTask, "data", this);
  ESP_LOGI(TAG, "Data task created");

  playlistLock = portMUX_INITIALIZER_UNLOCKED;
  playlisthandle = startTask(playlistTask, "playlist", this);
  ESP_LOGI(TAG, "Playlist task created");

  // startTask(debugTask, "debug", this);

  result = true;

exit:
  return result;
}

void classPlayer::setVolume(int volume) {
  // pVS1053->setVolume(volume);
}

/*
Only send when DREQ asserted.
Check for endedness and rising/falling clock sync, page 17.
Be aware of minimum clock pulse widths, which are based on CLKI cycles, page 18.
Library SPI maybe has too much overhead - write our own for this part?
*/

// Send data from the ring buffer to the VS1053.
void classPlayer::dataTask(void *pdata) {
  classPlayer *pthis = (classPlayer *)pdata;
  RingBuffer *pring = pthis->pBuffer;
  int count, chunk;

  while (true) {
    count = MIN(pring->available(), 10000);
    if (count > 0) {
      if (pring->head > pring->tail) {
        pthis->pVS1053b->playChunk(pring->pTail(), count);
      } else {
        chunk = MIN(pring->size - pring->tail, count);
        pthis->pVS1053b->playChunk(pring->pTail(), chunk);
        chunk = count - chunk;
        if (chunk > 0) {
          pthis->pVS1053b->playChunk(pring->pData, chunk);
        }
      }

      pring->tail += count;
      if (pring->tail > pring->size) {
        pring->tail -= pring->size;
      }

      if (!pthis->isPlaying) {
        pthis->pBuffer->clear();
        continue;
      }
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

  size = esp_http_client_get_content_length(pPlayer->httpClient);

  ESP_LOGI(TAG, "%d bytes total", size);
  processed = 0;
  pPlayer->isPlaying = true;

  while (processed < size) {
    if (pPlayer->stopPlay) {
      break;
    }

    space = pPlayer->pBuffer->space();
    if (space == 0) {
      vTaskDelay(50);
      continue;
    }

    /*start = millis();
    while ((available = wifiClient.available()) == 0) {
      if (millis() - start > 5000) {
        Serial.println("Timeout waiting for resource response");
        goto exit;
      }
      vTaskDelay(10);
    }*/

    // "count" is the total number of bytes we're going to add to the buffer.
    count = MIN(available, space);

    if (pPlayer->pBuffer->head >= pPlayer->pBuffer->tail) {
      // Write from the head to buffer end, then from buffer start to the tail if there's more to write.
      chunk = MIN(pPlayer->pBuffer->size - pPlayer->pBuffer->head, count);
      count = esp_http_client_read(pPlayer->httpClient, (char *)pPlayer->pBuffer->pHead(), chunk);

      chunk = count - chunk;
      if (chunk > 0) {
        esp_http_client_read(pPlayer->httpClient, (char *)pPlayer->pBuffer->pData, chunk);
      }
    } else {
      // Write everything at head.
      esp_http_client_read(pPlayer->httpClient, (char *)pPlayer->pBuffer->pHead(), count);
    }

    // Bump the buffer head but only set it once as that's what triggers the sending process.
    if (pPlayer->pBuffer->head + count > pPlayer->pBuffer->size) {
      pPlayer->pBuffer->head += count - pPlayer->pBuffer->size;
    } else {
      pPlayer->pBuffer->head += count;
    }

    /*if (!task_started && pPlayer->pBuffer->available() > 100000) {
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
    }*/

    /*if (task_started) {
      vTaskResume(dataHandle);
    }*/

    // Bump the total amount processed of the response.
    processed += count;

    taskYIELD();
  }

exit:
  // pPlayer->pHttpClient->end();
  ESP_LOGI(TAG, "Finished playing");
  pPlayer->isPlaying = false;

  // TODO Wait for buffer to be empty then delete the task.
  vTaskDelete(NULL);
}

void classPlayer::debugTask(void *pdata) {
  while (1) {
    classPlayer *pplayer = (classPlayer *)pdata;
    RingBuffer *pbuffer = pplayer->pBuffer;
    // Serial.println(pbuffer->available());
    vTaskDelay(100);
  }
}

// Start playing a track by requesting data from the server and creating a task to keep reading it into the ring buffer.
void classPlayer::startPlay() {
  esp_http_client_config_t config;

  wifiWait();
  ESP_LOGI(TAG, "WiFi ready");

  // Stop any existing play before starting the new one.
  if (isPlaying) {
    stopPlay = true;
    while (isPlaying) {
      vTaskDelay(10);
    }
    pBuffer->clear();
  }

  ESP_LOGI(TAG, "Requesting track %s from %s", currentTrack.title, currentTrack.resource);

  memset(&config, 0, sizeof config);
  config.url = currentTrack.resource;
  config.user_agent = "Plex radio";
  config.method = HTTP_METHOD_GET;
  httpClient = esp_http_client_init(&config);

  esp_http_client_open(httpClient, 0);
  esp_http_client_fetch_headers(httpClient);

  int code = esp_http_client_get_status_code(httpClient);
  ESP_LOGI(TAG, "Got code %d loading resource", code);
  if (code != 200) {
    goto exit;
  }

  stopPlay = false;
  startTask(readTask, "read", this);

exit:
  return;
}

/*
Keep a playlist as a queue of tracks. Play the next track when one is finished (maybe with pause).
*/

void classPlayer::playlistTask(void *pdata) {
  classPlayer *pplayer = (classPlayer *)pdata;

  while (1) {
    // Only start a track playing if there isn't one already playing (TODO), and the queue isn't empty.
    taskENTER_CRITICAL(&pplayer->playlistLock);

    if (pplayer->playlist.size() == 0) {
      taskEXIT_CRITICAL(&pplayer->playlistLock);
      vTaskSuspend(NULL);
      continue;
    }

    pplayer->currentTrack = pplayer->playlist.front();
    pplayer->playlist.pop();
    taskEXIT_CRITICAL(&pplayer->playlistLock);

    pplayer->startPlay();
  }
}

void classPlayer::addToPlaylist(Track *ptrack) {
  taskENTER_CRITICAL(&playlistLock);
  playlist.push(*ptrack);
  taskEXIT_CRITICAL(&playlistLock);
  vTaskResume(playlisthandle);
}

void classPlayer::clearPlaylist() {
  taskENTER_CRITICAL(&playlistLock);
  while (playlist.size() > 0) {
    playlist.pop();
  }
  taskEXIT_CRITICAL(&playlistLock);
}

void classPlayer::resetPlaylist(Track *ptrack) {
  taskENTER_CRITICAL(&playlistLock);
  clearPlaylist();
  addToPlaylist(ptrack);
  taskEXIT_CRITICAL(&playlistLock);
}

void classPlayer::stop() {
  if (isPlaying) {
    stopPlay = true;
    while (isPlaying) {
      vTaskDelay(10);
    }
  }
}