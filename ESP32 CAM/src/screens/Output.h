#pragma once

#include <LiquidCrystal_I2C.h>

#define OUTPUT_WIDTH    16
#define OUTPUT_HEIGHT   2

class classOutput {
 private:
  LiquidCrystal_I2C *pdisplay;

 public:
  classOutput();
  virtual ~classOutput();
  virtual void begin();
  virtual void addText(int x, int y, const char *ptext);
  virtual void setLine(int y, const char *ptext);
};

extern classOutput Output;
