#pragma once

#include <LiquidCrystal_I2C.h>

class classOutput {
 private:
  LiquidCrystal_I2C *pdisplay;
  int displayWidth, displayHeight;

 public:
  classOutput();
  virtual ~classOutput();
  virtual void begin();
  virtual void addText(int x, int y, const char *ptext);
};

extern classOutput Output;
