#include "button.h"

#include "../utils.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Button::Button(int lightPin) {
  this->lightPin = lightPin;
  gpio_set_direction((gpio_num_t)lightPin, GPIO_MODE_OUTPUT);
}

void Button::begin(State initialState) {
  setState(initialState);

  startTask(buttonTask, "button", this);
}

void Button::setState(State newState) {
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

void Button::show(bool show) {
  if (show) {
    gpio_set_level((gpio_num_t)lightPin, 1);
  } else {
    gpio_set_level((gpio_num_t)lightPin, 0);
  }
}

void Button::buttonTask(void *pparams) {
  Button *pthis = (Button *)pparams;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(500));
    if (pthis->state == Flashing) {
      pthis->showing = !pthis->showing;
      pthis->show(pthis->showing);
    }
  }
}