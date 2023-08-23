#include "Tracks.h"

#include <Arduino.h>

#include "../data/data.h"
#include "player.h"

classTracks Tracks;

void classTracks::setAlbum(Album *palbum) {
  pAlbum = palbum;
}

void classTracks::activate() {
  int n;

  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);

  count = Data.getTracks(pAlbum->id, &ptracks);
  current = 0;
  Output.setLine(0, pAlbum->title);
  showCurrent();
}

void classTracks::deactivate() {
  EventManager.removeListener(this);

  delete[] ptracks;
}

bool classTracks::onEvent(Event *pevent) {
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

void classTracks::handleEncoderEvent(EncoderEvent *pevent) {
  current += pevent->step;
  if (current < 0) current = 0;
  if (current > count - 1) current = count - 1;
  showCurrent();
}

void classTracks::handleSwitchEvent(SwitchEvent *pevent) {
  if (pevent->pressed) {
    Track *ptrack = ptracks + current;
    Player.play(ptrack->resource);
    Serial.printf("Encoder pressed on track %s\n", ptrack->resource);
  }
}

void classTracks::showCurrent() {
  Track *ptrack = ptracks + current;
  Output.setLine(1, ptrack->title);
}