#include "play.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pins.h"
#include "utils.h"

namespace PlayButton {

State state;
bool showing;

void show(bool show) {
  if (show) {
    gpio_set_level((gpio_num_t)PLAY_LIGHT, 1);
  } else {
    gpio_set_level((gpio_num_t)PLAY_LIGHT, 0);
  }
}

void ButtonTask(void *pparams) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(500));
    if (state == Flashing) {
      showing = !showing;
      show(showing);
    }
  }
}

void setState(State newState) {
  state = newState;
  switch (state) {
    case Off:
      show(false);
      break;
    case On:
      show(true);
      break;
    case Flashing:
      show(false);
      showing = false;
      break;
  }
}

void begin(State initialState) {
  gpio_set_direction((gpio_num_t)PLAY_LIGHT, GPIO_MODE_OUTPUT);
  setState(initialState);

  startTask(ButtonTask, "play_button");
}

}  // namespace PlayButton