#include "player.h"

#include "buttons/play.h"
#include "constants.h"
#include "driver/gpio.h"
#include "esp32-hal-log.h"
#include "esp_http_client.h"
#include "freertos/stream_buffer.h"
#include "playlist.h"
#include "ring.h"
#include "utils.h"
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

#define STOP_PLAY 0x0001
#define PLAYLIST_READY 0x0002
#define DATA_AVAILABLE 0x0004
#define ROOM_AVAILABLE 0x0008
#define BUFFER_EMPTY 0x0010
#define FETCH_COMPLETE 0x0020

VS1053b *pVS1053b;

EventGroupHandle_t eventGroup;

RingBuffer *pRing;
bool paused, stopShowing;
Playlist *pPlaylist;

void mute(bool mute) {
  // gpio_set_level((gpio_num_t)AUDIO_MUTE, !mute);
  // setVolume(mute ? 0 : 100);
}

void playTask(void *pparms) {
  uint8_t buffer[32];

  // TODO How to know when file has finished? Does it matter? Maybe task runs forever.
  ESP_LOGD(TAG, "Play task starting");

  while (1) {
    // Wait for 32 bytes of sound data to be available.
    // TODO Use timeout and report error.
    xEventGroupWaitBits(eventGroup, DATA_AVAILABLE, CLEAR_ON_EXIT, WAIT_ALL, portMAX_DELAY);
    pRing->get(buffer, 32);

    pVS1053b->sendChunk(buffer, 32);
  }
}

void fetchTask(void *pparam) {
  int length, count, sent, code, chunk;
  bool done;
  esp_http_client_config_t httpConfig;
  esp_http_client_handle_t httpClient;
  uint8_t *pbuffer;
  Track track;
  EventBits_t reason;

  ESP_LOGI(TAG, "Fetch task starting");

  pbuffer = new uint8_t[10240];

  while (1) {
    xEventGroupWaitBits(eventGroup, PLAYLIST_READY | BUFFER_EMPTY, NO_CLEAR, WAIT_ALL, portMAX_DELAY);
    ESP_LOGD(TAG, "Playlist ready and buffer empty");

    pPlaylist->get(&track);
    xEventGroupClearBits(eventGroup, STOP_PLAY);
    xEventGroupClearBits(eventGroup, FETCH_COMPLETE);
    PlayButton::setState(PlayButton::State::Flashing);

    wifiWait();
    ESP_LOGI(TAG, "WiFi connected for playing %s", track.resource);

    memset(&httpConfig, 0, sizeof httpConfig);
    httpConfig.url = track.resource;
    httpConfig.user_agent = "Plex radio";
    httpConfig.method = HTTP_METHOD_GET;
    httpClient = esp_http_client_init(&httpConfig);

    // TODO If status code is not 200 then retry, ideally warning user. If it fails consistently, or if read fails later, then abort play?
    do {
      esp_http_client_open(httpClient, 0);
      length = esp_http_client_fetch_headers(httpClient);
      code = esp_http_client_get_status_code(httpClient);
      ESP_LOGI(TAG, "Status code %d, content length %d", code, length);
    } while (code != 200);

    PlayButton::setState(PlayButton::State::On);

  done = false;
    while (!done) {
      // TODO Use timeout and report error.
      // TODO Ideally interrupt a blocking HTTP read.
      count = esp_http_client_read(httpClient, (char *)pbuffer, 10240);
      if (xEventGroupGetBits(eventGroup) & STOP_PLAY) {
        break;
      }      
      if (count == 0) {
        break;
      }

      while (count > 0) {
        reason = xEventGroupWaitBits(eventGroup, ROOM_AVAILABLE | STOP_PLAY, CLEAR_ON_EXIT, WAIT_ONE, portMAX_DELAY);
        if (reason & STOP_PLAY) {
          ESP_LOGI(TAG, "Stopping fetch");
          done = true;
          break;
        }
        chunk = MIN(pRing->room(), count);
        pRing->put(pbuffer, chunk);
        count -= chunk;
      }
    }

    esp_http_client_close(httpClient);
    esp_http_client_cleanup(httpClient);

    ESP_LOGD(TAG, "Fetch complete");
    xEventGroupSetBits(eventGroup, FETCH_COMPLETE);
  }
}

void skipToNext() {
  xEventGroupSetBits(eventGroup, STOP_PLAY);
  xEventGroupWaitBits(eventGroup, FETCH_COMPLETE, NO_CLEAR, WAIT_ONE, portMAX_DELAY);
  pRing->clear();
}

void playTrack(Track *ptrack) {
  pPlaylist->clear();
  xEventGroupSetBits(eventGroup, STOP_PLAY);
  xEventGroupWaitBits(eventGroup, FETCH_COMPLETE, NO_CLEAR, WAIT_ONE, portMAX_DELAY);
  pRing->clear();
  pPlaylist->put(ptrack);
}

void pausePlay(bool pause) {
  if (pause) {
    // Stop the playing task, the fetch task will evetually block too if it's running.
    // stopPlayTask();
  } else {
    // Start the playing task, the fetch task will unblock when it can and add to the buffer again if it's still running.
    // startPlayTask();
  }
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

bool begin() {
  bool result = false;

  eventGroup = xEventGroupCreate();

  pPlaylist = new Playlist(eventGroup);
  pPlaylist->setReady(PLAYLIST_READY);

  pVS1053b = new VS1053b(VS1053_CS, VS1053_DCS, VS1053_DREQ, VS1053_RESET);
  pVS1053b->begin();

  // This will get allocated in PSRAM, mapped to normal address space.
  pRing = new RingBuffer(4000000, eventGroup);
  pRing->setThreshold(32, DATA_AVAILABLE);
  pRing->setRoom(10240, ROOM_AVAILABLE);
  pRing->setEmpty(BUFFER_EMPTY);

  // gpio_set_direction((gpio_num_t)AUDIO_MUTE, GPIO_MODE_OUTPUT);

  xEventGroupSetBits(eventGroup, FETCH_COMPLETE);

  startTask(playTask, "play");
  startTask(fetchTask, "fetch");

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