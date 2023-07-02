#pragma once

#include "EventManager.h"
#include "EventReceiver.h"
#include "Screen.h"
#include "../data/album.h"

class classAlbums : public Screen, EventReceiver {
 private:
  int count, current;
  Album *palbums;
  void showCurrent();
  void handleEncoderEvent(EncoderEvent*);

 public:
  virtual void activate();
  virtual void deactivate();
  virtual bool onEvent(Event* pevent);
};

extern classAlbums Albums;
