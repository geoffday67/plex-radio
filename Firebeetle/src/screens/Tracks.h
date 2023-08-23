#pragma once

#include "../data/album.h"
#include "../data/track.h"
#include "EventManager.h"
#include "EventReceiver.h"
#include "Screen.h"

class classTracks : public Screen, EventReceiver {
 private:
  int count, current;
  Album *pAlbum;
  Track *ptracks;
  void showCurrent();
  void handleEncoderEvent(EncoderEvent *);
  void handleSwitchEvent(SwitchEvent *);

 public:
  void setAlbum(Album *);

  virtual void activate();
  virtual void deactivate();
  virtual bool onEvent(Event *pevent);
};

extern classTracks Tracks;