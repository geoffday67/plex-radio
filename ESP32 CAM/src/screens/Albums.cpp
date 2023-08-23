#include "Albums.h"

#include <Arduino.h>

#include "../data/data.h"
#include "Tracks.h"
#include "utils.h"

classAlbums Albums;

Scroll::Scroll(char *ptext, int line) {
  this->ptext = strdup(ptext);
  this->line = line;
}

Scroll::~Scroll() {
  // vTaskDelete(taskHandle);
  free(ptext);
}

void Scroll::begin() {
  Output.setLine(line, ptext);
  // this->taskHandle = startTask(scrollCode, "Scroll", this);
}

void Scroll::scrollCode(void *pdata) {
  Scroll *pscroll = (Scroll *)pdata;
  char *ptext = pscroll->ptext;
  int start, length = strlen(ptext), line = pscroll->line;
  Serial.printf("Scrolling %s on line %d\n", ptext, line);

  for (start = 0; length - start >= OUTPUT_WIDTH; start++) {
    Output.setLine(line, ptext + start);
    vTaskDelay(100);
  }

  Output.setLine(line, ptext);

  vTaskDelete(NULL);
}

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

  pscroll1 = 0;
  pscroll2 = 0;
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
    Serial.println("Encoder pressed");
    this->deactivate();
    Tracks.setAlbum(palbums + current);
    Tracks.activate();
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