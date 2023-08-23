#pragma once

#include "../data/album.h"
#include "EventManager.h"
#include "EventReceiver.h"
#include "Screen.h"

class Scroll {
 private:
  static void scrollCode(void*);

 public:
  Scroll(char* ptext, int line);
  ~Scroll();
  void begin();
  char* ptext;
  int line;
  TaskHandle_t taskHandle;
};

class classAlbums : public Screen, EventReceiver {
 private:
  int count, current;
  Album* palbums;
  void showCurrent();
  void handleEncoderEvent(EncoderEvent*);
  void handleSwitchEvent(SwitchEvent*);
  Scroll *pscroll1, *pscroll2;

 public:
  virtual void activate();
  virtual void deactivate();
  virtual bool onEvent(Event* pevent);
};

extern classAlbums Albums;
