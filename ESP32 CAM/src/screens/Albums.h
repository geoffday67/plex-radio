#pragma once

#include "../data/album.h"
#include "EventManager.h"
#include "EventReceiver.h"
#include "Screen.h"
#include "Scroll.h"

class classAlbums : public Screen, EventReceiver {
 private:
  int count, current;
  Album* palbums;
  void showCurrent();
  void handleEncoderEvent(EncoderEvent*);
  void handleSwitchEvent(SwitchEvent*);
  void handlePlayEvent(PlayEvent*);
  void handleBackEvent(BackEvent*);
  Scroll *pscroll1, *pscroll2;

 public:
  virtual void activate();
  virtual void deactivate();
  virtual bool onEvent(Event* pevent);
};

extern classAlbums Albums;
