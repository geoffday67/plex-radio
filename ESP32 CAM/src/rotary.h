#pragma once

#include <Arduino.h>

class Rotary {
 private:
  int clk, dt, button;
  uint8_t prevNextCode;
  uint16_t store;

 public:
  Rotary(int clk, int dt, int button);
  void begin();
  int getDirection();
};