#include "Tracks.h"

#include <Arduino.h>

#include "../data/data.h"
#include "../player.h"
#include "Albums.h"
#include "utils.h"

classTracks Tracks;

void classTracks::setAlbum(Album *palbum) {
  pAlbum = palbum;
}

void classTracks::activate() {
  int n;
  char title[TITLE_SIZE];

  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);
  EventManager.addListener(EVENT_BACK, this);

  count = Data.getTracks(pAlbum->id, &ptracks);
  current = 0;

  decodeUTF8(title, pAlbum->title);
  pscroll1 = new Scroll(title, 0);
  pscroll1->begin();
  pscroll2 = 0;

  showCurrent();
}

void classTracks::deactivate() {
  EventManager.removeListener(this);

  delete pscroll1;
  delete pscroll2;
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
    case EVENT_BACK:
      handleBackEvent((BackEvent *)pevent);
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
    Serial.printf("Encoder pressed on track %s\n", ptrack->title);
    Player::resetPlaylist(ptrack);
  }
}

void classTracks::handleBackEvent(BackEvent *pevent) {
  if (pevent->pressed) {
    Serial.println("Back pressed");
    this->deactivate();
    Albums.activate();
  }
}

void classTracks::showCurrent() {
  Track *ptrack = ptracks + current;
  char title[TITLE_SIZE];

  delete pscroll2;
  decodeUTF8(title, ptrack->title);
  pscroll2 = new Scroll(title, 1);
  pscroll2->begin();
}