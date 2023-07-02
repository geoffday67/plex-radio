#include "rotary.h"

Rotary::Rotary(int clk, int dt, int blah) {
  this->clk = clk;
  this->dt = dt;
  this->button = blah;
  prevNextCode = 0;
  store = 0;
}

void Rotary::begin() {
  pinMode(clk, INPUT_PULLUP);
  pinMode(dt, INPUT_PULLUP);
}

int Rotary::getDirection() {
  static int8_t rot_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

  prevNextCode <<= 2;
  if (digitalRead(dt)) prevNextCode |= 0x02;
  if (digitalRead(clk)) prevNextCode |= 0x01;
  prevNextCode &= 0x0f;

  // If valid then store as 16 bit data.
  if (rot_enc_table[prevNextCode]) {
    store <<= 4;
    store |= prevNextCode;
    // if (store==0xd42b) return 1;
    // if (store==0xe817) return -1;
    if ((store & 0xff) == 0x2b) return -1;
    if ((store & 0xff) == 0x17) return 1;
  }
  return 0;
}