#pragma once

#include <LiquidCrystal_I2C.h>

#define OUTPUT_WIDTH    16
#define OUTPUT_HEIGHT   2

class classOutput {
 private:
  LiquidCrystal_I2C *pdisplay;
  SemaphoreHandle_t mutex;

 public:
  classOutput();
  virtual ~classOutput();
  virtual void begin();
  virtual void addText(int x, int y, const char *ptext);
  virtual void setLine(int y, const char *ptext);
  virtual void clear();
};

extern classOutput Output;
