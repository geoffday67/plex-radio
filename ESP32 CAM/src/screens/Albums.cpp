#include "Albums.h"

#include <Arduino.h>

#include "../data/data.h"
#include "../player.h"
#include "Menu.h"
#include "Tracks.h"
#include "buttons/play.h"

classAlbums Albums;

void classAlbums::activate() {
  int n;

  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);
  EventManager.addListener(EVENT_PLAY, this);
  EventManager.addListener(EVENT_BACK, this);

  // Get all the albums (175 albums => 60K ish).
  // Keep index/pointer to current one, update and display as encoder events received.

  if (!palbums) {
    count = Data.getAlbums(&palbums);
    Serial.printf("%d albums fetched\n", count);
    current = 0;
  }

  pscroll1 = 0;
  pscroll2 = 0;
  showCurrent();
}

void classAlbums::deactivate() {
  EventManager.removeListener(this);

  delete pscroll1;
  delete pscroll2;
}

bool classAlbums::onEvent(Event *pevent) {
  switch (pevent->type) {
    case EVENT_ENCODER:
      handleEncoderEvent((EncoderEvent *)pevent);
      break;
    case EVENT_SWITCH:
      handleSwitchEvent((SwitchEvent *)pevent);
      break;
    case EVENT_PLAY:
      handlePlayEvent((PlayEvent *)pevent);
      break;
    case EVENT_BACK:
      handleBackEvent((BackEvent *)pevent);
      break;
  }
  return true;
}

void classAlbums::handleEncoderEvent(EncoderEvent *pevent) {
  int previous = current;
  current += pevent->step;
  if (current < 0) current = 0;
  if (current > count - 1) current = count - 1;
  if (previous != current) {
    showCurrent();
  }
}

void classAlbums::handleSwitchEvent(SwitchEvent *pevent) {
  if (pevent->pressed) {
    PlayButton::setState(Button::State::Flashing);
    Album *palbum = palbums + current;
    Serial.printf("Encoder pressed on album %s\n", palbum->title);
    this->deactivate();
    Tracks.setAlbum(palbum);
    Tracks.activate();
  }
}

void classAlbums::handlePlayEvent(PlayEvent *pevent) {
  int count;
  Track *ptracks;

  if (pevent->pressed) {
    PlayButton::setState(Button::State::Flashing);
    Album *palbum = palbums + current;
    Serial.printf("Play pressed on album %s\n", palbum->title);
    count = Data.getTracks(palbum->id, &ptracks);
    Player::playTracks(ptracks, count);
  }
}

void classAlbums::handleBackEvent(BackEvent *pevent) {
  if (pevent->pressed) {
    Serial.println("Back pressed");
    if (palbums) {
      delete[] palbums;
      palbums = NULL;
    }
    this->deactivate();
    Menu.activate();
  }
}

void classAlbums::showCurrent() {
  Album *palbum = palbums + current;

  delete pscroll1;
  pscroll1 = new Scroll(palbum->artist, 0);
  pscroll1->begin();
  delete pscroll2;
  pscroll2 = new Scroll(palbum->title, 1);
  pscroll2->begin();
}