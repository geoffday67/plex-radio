#pragma once

#include <Arduino.h>

class Scroll {
 private:
  static void scrollCode(void*);
  SemaphoreHandle_t semaphoreQuit;

 public:
  Scroll(char* ptext, int line);
  ~Scroll();
  void begin();
  char* ptext;
  int line;
  TaskHandle_t taskHandle = 0;
};
