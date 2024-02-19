#include "Output.h"

#include "../utils.h"

#define SDA_PIN 13
#define SCL_PIN 14

classOutput Output;

classOutput::classOutput() {
  pdisplay = new LiquidCrystal_I2C(0x27, OUTPUT_WIDTH, OUTPUT_HEIGHT);
  mutex = xSemaphoreCreateMutex();
}

classOutput::~classOutput() {}

void classOutput::begin() {
  pdisplay->init(SDA_PIN, SCL_PIN);
  pdisplay->backlight();
  loadCustomChars();
}

void classOutput::loadCustomChars() {
  // Custom accent characters since the LCD uses the Japanese ROM.
  uint8_t o_umlaut[] = {0x0A, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00};
  uint8_t up_arrow[] = {0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x04, 0x00};
  uint8_t down_arrow[] = {0x04, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00};
  uint8_t e_acute[] = {0x02, 0x04, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0x00};
  uint8_t right_arrow[] = {0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00, 0x00};
  uint8_t left_arrow[] = {0x00, 0x04, 0x08, 0x1F, 0x08, 0x04, 0x00, 0x00};

  pdisplay->createChar(O_UMLAUT, o_umlaut);
  pdisplay->createChar(E_ACUTE, e_acute);
  pdisplay->createChar(UP_ARROW, up_arrow);
  pdisplay->createChar(DOWN_ARROW, down_arrow);
  pdisplay->createChar(RIGHT_ARROW, right_arrow);
  pdisplay->createChar(LEFT_ARROW, left_arrow);
}

void classOutput::addText(int x, int y, const char *ptext) {
  if (xSemaphoreTake(mutex, 2000 * portTICK_PERIOD_MS) == pdTRUE) {
    pdisplay->setCursor(x, y);
    pdisplay->print(ptext);
    xSemaphoreGive(mutex);
  }
}

void classOutput::addChar(int x, int y, char c) {
  if (xSemaphoreTake(mutex, 2000 * portTICK_PERIOD_MS) == pdTRUE) {
    pdisplay->setCursor(x, y);
    pdisplay->write(c);
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

void classOutput::centreText(int y, const char *ptext) {
  char line[OUTPUT_WIDTH + 1];
  int padding = (OUTPUT_WIDTH - strlen(ptext)) / 2;

  memset(line, ' ', OUTPUT_WIDTH);
  line[OUTPUT_WIDTH] = 0;
  memcpy(line + padding, ptext, strlen(ptext));
  addText(0, y, line);
}

void classOutput::clear() {
  if (xSemaphoreTake(mutex, 2000 * portTICK_PERIOD_MS) == pdTRUE) {
    pdisplay->clear();
    xSemaphoreGive(mutex);
  }
}