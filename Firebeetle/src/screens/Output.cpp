#include "Output.h"

classOutput Output;

classOutput::classOutput() {
  displayHeight = 2;
  displayWidth = 16;
  pdisplay = new LiquidCrystal_I2C(0x27, displayHeight, displayWidth);
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