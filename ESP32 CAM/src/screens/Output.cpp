#include "Output.h"

#include "../utils.h"

#define SDA_PIN 13
#define SCL_PIN 27

classOutput Output;

classOutput::classOutput() {
  pdisplay = new LiquidCrystal_I2C(0x27, OUTPUT_WIDTH, OUTPUT_HEIGHT);
  mutex = xSemaphoreCreateMutex();
}

classOutput::~classOutput() {}

void classOutput::begin() {
  pdisplay->init(SDA_PIN, SCL_PIN);
  pdisplay->backlight();
}

void classOutput::addText(int x, int y, const char *ptext) {
  if (xSemaphoreTake(mutex, 2000 * portTICK_PERIOD_MS) == pdTRUE) {
    pdisplay->setCursor(x, y);
    pdisplay->print(ptext);
    xSemaphoreGive(mutex);
  }
}

void classOutput::setLine(int y, const char *ptext) {
  char line[OUTPUT_WIDTH + 1];

  memset(line, ' ', OUTPUT_WIDTH);
  line[OUTPUT_WIDTH] = 0;
  memcpy(line, ptext, MIN(strlen(ptext), OUTPUT_WIDTH));
  addText(0, y, line);
}

void classOutput::clear() {
  if (xSemaphoreTake(mutex, 2000 * portTICK_PERIOD_MS) == pdTRUE) {
    pdisplay->clear();
  }
}