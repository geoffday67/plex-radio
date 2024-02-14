#pragma once

#include "../data/album.h"
#include "../data/track.h"
#include "EventManager.h"
#include "EventReceiver.h"
#include "Screen.h"
#include "Scroll.h"

class classTracks : public Screen, EventReceiver {
 private:
  int count, current;
  Album *pAlbum;
  Track *ptracks;
  void showCurrent();
  void handleEncoderEvent(EncoderEvent *);
  void handleSwitchEvent(SwitchEvent *);
  void handleBackEvent(BackEvent *);
  void handlePlayEvent(PlayEvent *);
  Scroll *pscroll1, *pscroll2;

 public:
  void setAlbum(Album *);

  virtual void activate();
  virtual void deactivate();
  virtual bool onEvent(Event *pevent);
};

extern classTracks Tracks;