#include "Albums.h"

#include <Arduino.h>

#include "../data/data.h"
#include "Tracks.h"

classAlbums Albums;

void classAlbums::activate() {
  int n;

  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);

  // Get all the albums (175 albums => 60K ish).
  // Keep index/pointer to current one, update and display as encoder events received.

  if (!palbums) {
    count = Data.getAlbums(&palbums);
    current = 0;
  }
  showCurrent();
}

void classAlbums::deactivate() {
  EventManager.removeListener(this);
}

bool classAlbums::onEvent(Event *pevent) {
  switch (pevent->type) {
    case EVENT_ENCODER:
      handleEncoderEvent((EncoderEvent *)pevent);
      break;
    case EVENT_SWITCH:
      handleSwitchEvent((SwitchEvent *)pevent);
      break;
  }
  return true;
}

void classAlbums::handleEncoderEvent(EncoderEvent *pevent) {
  current += pevent->step;
  if (current < 0) current = 0;
  if (current > count - 1) current = count - 1;
  showCurrent();
}

void classAlbums::handleSwitchEvent(SwitchEvent *pevent) {
  if (pevent->pressed) {
    this->deactivate();
    Tracks.setAlbum(palbums + current);
    Tracks.activate();
  }
}

void classAlbums::showCurrent() {
  Album *palbum = palbums + current;
  Output.setLine(0, palbum->artist);
  Output.setLine(1, palbum->title);
}