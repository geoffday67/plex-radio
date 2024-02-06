#include "Albums.h"

#include <Arduino.h>

#include "../data/data.h"
#include "Tracks.h"
#include "utils.h"

classAlbums Albums;

void classAlbums::activate() {
  int n;

  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);

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
    Album *palbum = palbums + current;
    Serial.printf("Encoder pressed on album %s\n", palbum->title);
    this->deactivate();
    Tracks.setAlbum(palbum);
    Tracks.activate();
  }
}

void classAlbums::showCurrent() {
  Album *palbum = palbums + current;
  char artist[ARTIST_SIZE];
  char title[TITLE_SIZE];

  /*Serial.println(palbum->artist);
  for (int n = 0; n < strlen(palbum->artist); n++) {
    if (palbum->artist[n] == 0xC3) {
      palbum->artist[n] = 148;
    }
    Serial.printf("[0x%02X]", palbum->artist[n]);
  }
  Serial.println();*/

  /*
    Text (e.g. title) is encoded as UTF-8 which we need to translate to the right characters for display.
    The character ROM on the LCD driver is the Japanese font so we use custom chars for the accent chars.
    We only handle 2 byte characters.
  */

  delete pscroll1;
  decodeUTF8(artist, palbum->artist);
  pscroll1 = new Scroll(artist, 0);
  pscroll1->begin();
  delete pscroll2;
  decodeUTF8(title, palbum->title);
  pscroll2 = new Scroll(title, 1);
  pscroll2->begin();
}