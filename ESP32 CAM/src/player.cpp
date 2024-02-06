#include "player.h"

#include <esp_task_wdt.h>

#include <queue>

#include "constants.h"
#include "driver/gpio.h"
#include "esp32-hal-log.h"
#include "esp_http_client.h"
#include "freertos/stream_buffer.h"
#include "utils.h"
// #include "vs1053b-patches-flac.h"
#include "vs1053b.h"

static const char *TAG = "Player";

extern void wifiWait(void);
extern EventGroupHandle_t plexRadioGroup;
extern void showStop(bool show);

namespace Player {

#define AUDIO_MUTE 25

#define VS1053_CS 21
#define VS1053_DCS 22
#define VS1053_DREQ 15
#define VS1053_RESET 4

#define STOP_FETCH 0x0001
#define FETCH_FINISHED 0x0002
#define PLAY_READY 0x0004
#define PLAY_FINISHED 0x0008

VS1053b *pVS1053b;

EventGroupHandle_t eventGroup;

std::queue<Track> playlist;
Track currentTrack;

StaticStreamBuffer_t audioStaticStream;
StreamBufferHandle_t audioStream;
uint8_t *paudioStreamBuffer;
esp_http_client_handle_t httpClient;
uint8_t *pHttpBuffer;
bool paused, stoppingFetchTask, stoppingPlayTask, stopShowing;

void mute(bool mute) {
  // gpio_set_level((gpio_num_t)AUDIO_MUTE, !mute);
  // setVolume(mute ? 0 : 100);
}

void playTask(void *pparms) {
  uint8_t buffer[32];
  int count;

  // TODO How to know when file has finished? Does it matter? Maybe task runs forever.
  ESP_LOGD(TAG, "Play task starting");

  while (1) {
    // Wait for 32 bytes of sound data to be available.
    // Only wait for 1 second in case we're stopping.
    // TODO Use timeout and report error.
    count = xStreamBufferReceive(audioStream, buffer, 32, pdMS_TO_TICKS(1000));

    // If we're stopping then quit the loop.
    if (stoppingPlayTask) {
      break;
    }

    if (count > 0) {
      pVS1053b->sendChunk(buffer, count);
    }
  }

  ESP_LOGD(TAG, "Play task quitting");

  // Tell observers that we're done.
  xEventGroupSetBits(eventGroup, PLAY_FINISHED);
  vTaskDelete(NULL);
}

void fetchTask(void *pparam) {
  char *purl = (char *)pparam;
  int length, count, sent;
  bool done;
  esp_http_client_config_t httpConfig;

  ESP_LOGI(TAG, "Fetch task starting for %s", purl);

  pHttpBuffer = new uint8_t[10240];

  memset(&httpConfig, 0, sizeof httpConfig);
  httpConfig.url = purl;
  httpConfig.user_agent = "Plex radio";
  httpConfig.method = HTTP_METHOD_GET;
  httpClient = esp_http_client_init(&httpConfig);
  // TODO Check for error and report it back.

  esp_http_client_open(httpClient, 0);
  length = esp_http_client_fetch_headers(httpClient);
  ESP_LOGI(TAG, "Status code %d, content length %d", esp_http_client_get_status_code(httpClient), length);

  done = false;
  while (!done) {
    count = esp_http_client_read(httpClient, (char *)pHttpBuffer, 10240);
    if (count == 0) {
      break;
    }

    if (stoppingFetchTask) {
      break;
    }

    // TODO Use timeout and report error.
    // Timeout after a second so we can check if we're stopping.
    sent = 0;
    do {
      sent += xStreamBufferSend(audioStream, pHttpBuffer, count, pdMS_TO_TICKS(1000));
      if (stoppingFetchTask) {
        done = true;
        break;
      }
    } while (sent != count);
  }

  esp_http_client_close(httpClient);
  esp_http_client_cleanup(httpClient);
  delete[] pHttpBuffer;

  ESP_LOGD(TAG, "Fetch task quitting");

  // Tell observers that we're done.
  xEventGroupSetBits(eventGroup, FETCH_FINISHED);
  vTaskDelete(NULL);
}

void startPlayTask() {
  if (audioStream) {
    xEventGroupClearBits(eventGroup, PLAY_FINISHED);
    stoppingPlayTask = false;
    startTask(playTask, "play");

    stopShowing = true;
    showStop(true);
  }
}

void stopPlayTask() {
  uint8_t dummy[32];

  stoppingPlayTask = true;

  // If there's an audio stream then write 32 bytes to it to make sure play task is unblocked.
  if (audioStream) {
    memset(dummy, 0, 32);
    // ONLY A SINGLE PROCESS CAN SEND TO THE STREAM!!!
    //xStreamBufferSend(audioStream, dummy, 32, portMAX_DELAY);
  }

  // Finished bit will already be set if previous play task is finished or never started.
  xEventGroupWaitBits(eventGroup, PLAY_FINISHED, pdFALSE, pdFALSE, pdMS_TO_TICKS(5000));
  ESP_LOGD(TAG, "Play task stopped");
}

void startFetchTask(char *purl) {
  if (audioStream) {
    xEventGroupClearBits(eventGroup, FETCH_FINISHED);
    stoppingFetchTask = false;
    startTask(fetchTask, "fetch", purl);
  }
}

void stopFetchTask() {
  stoppingFetchTask = true;

  // TODO How to make sure fetch task is unblocked? Only applies if fetch has not completed as otherwise task is already stopped.
  xEventGroupWaitBits(eventGroup, FETCH_FINISHED, pdFALSE, pdFALSE, 5000);
  ESP_LOGD(TAG, "Fetch task stopped");
}

void stopPlay() {
  uint8_t dummy[32];

  // TODO Mute audio while changing tasks/stream.
  mute(true);

  stopPlayTask();
  stopFetchTask();

  if (audioStream) {
    vStreamBufferDelete(audioStream);
    audioStream = 0;
    ESP_LOGD(TAG, "Audio stream deleted");
  } else {
    ESP_LOGD(TAG, "Audio stream not yet created");
  }
}

void pausePlay(bool pause) {
  if (pause) {
    // Stop the playing task, the fetch task will evetually block too if it's running.
    stopPlayTask();
  } else {
    // Start the playing task, the fetch task will unblock when it can and add to the buffer again if it's still running.
    startPlayTask();
  }
}

void playUrl(char *purl) {
  wifiWait();
  ESP_LOGI(TAG, "WiFi connected");

  audioStream = xStreamBufferCreateStatic(3000000, 32, paudioStreamBuffer, &audioStaticStream);
  ESP_LOGD(TAG, "Audio stream created");

  paused = false;
  mute(false);

  startPlayTask();
  startFetchTask(purl);
}

void stopButtonTask(void *pparams) {
  uint16_t e;

  while (1) {
    if (xEventGroupWaitBits(plexRadioGroup, STOP_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(500)) & STOP_BIT) {
      paused = !paused;
      if (paused) {
        stopShowing = false;
        showStop(false);
      }
      Player::pausePlay(paused);
      continue;
    }

    // We timed out, toggle the indicator if we're currently pausing.
    if (paused) {
      stopShowing = !stopShowing;
      showStop(stopShowing);
    }
  }
}

void playNext() {
  if (playlist.size() > 0) {
    currentTrack = playlist.front();
    playlist.pop();
    stopPlay();
    playUrl(currentTrack.resource);
  }
}

void addToPlaylist(Track *ptrack) {
  playlist.push(*ptrack);
  playNext();
}

void clearPlaylist() {
  while (playlist.size() > 0) {
    playlist.pop();
  }
}

void resetPlaylist(Track *ptrack) {
  clearPlaylist();
  addToPlaylist(ptrack);
}

bool begin() {
  bool result = false;

  eventGroup = xEventGroupCreate();

  // Mark the tasks as finished so a call to stop it returns immediately.
  xEventGroupSetBits(eventGroup, PLAY_FINISHED);
  xEventGroupSetBits(eventGroup, FETCH_FINISHED);

  pVS1053b = new VS1053b(VS1053_CS, VS1053_DCS, VS1053_DREQ, VS1053_RESET);
  pVS1053b->begin();

  // This will get allocated in PSRAM, mapped to normal address space.
  paudioStreamBuffer = new uint8_t[3000000];

  // gpio_set_direction((gpio_num_t)AUDIO_MUTE, GPIO_MODE_OUTPUT);

  // Watch for presses on the "stop" button.
  startTask(stopButtonTask, "stop_button");

  result = true;

exit:
  return result;
}

void setVolume(int volume) {
  pVS1053b->setVolume(volume);
}

}  // namespace Player