#pragma once

#include <LiquidCrystal_I2C.h>

#define OUTPUT_WIDTH    16
#define OUTPUT_HEIGHT   2

#define O_UMLAUT 1
#define E_ACUTE 2
#define UP_ARROW 3
#define DOWN_ARROW 4
#define RIGHT_ARROW 5
#define LEFT_ARROW 6

class classOutput {
 private:
  LiquidCrystal_I2C *pdisplay;
  SemaphoreHandle_t mutex;
  void loadCustomChars();

 public:
  classOutput();
  virtual ~classOutput();
  virtual void begin();
  virtual void addText(int x, int y, const char *ptext);
  virtual void addChar(int x, int y, char c);
  virtual void setLine(int y, const char *ptext);
  virtual void clear();
};

extern classOutput Output;
