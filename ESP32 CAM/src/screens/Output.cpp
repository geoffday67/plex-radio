#include "Output.h"

#include "../utils.h"

classOutput Output;

classOutput::classOutput() {
  pdisplay = new LiquidCrystal_I2C(0x27, OUTPUT_WIDTH, OUTPUT_HEIGHT);
}

classOutput::~classOutput() {}

void classOutput::begin() {
  pdisplay->init();
  pdisplay->backlight();
}

void classOutput::addText(int x, int y, const char *ptext) {
  pdisplay->setCursor(x, y);
  pdisplay->print(ptext);
}

void classOutput::setLine(int y, const char *ptext) {
  char line[OUTPUT_WIDTH + 1];

  memset(line, ' ', OUTPUT_WIDTH);
  line[OUTPUT_WIDTH] = 0;
  memcpy(line, ptext, MIN(strlen(ptext), OUTPUT_WIDTH));
  addText(0, y, line);
}
