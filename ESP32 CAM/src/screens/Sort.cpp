#include "Sort.h"

#include <Arduino.h>

classSort Sort;

classSort::classSort() {
  current = None;
}

void classSort::activate() {
  EventManager.addListener(EVENT_ENCODER, this);

  if (current == None) {
    current = Title;
  }

  showCurrent();
}

void classSort::deactivate() {
  EventManager.removeListener(this);
}

bool classSort::onEvent(Event *pevent) {
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

void classSort::handleEncoderEvent(EncoderEvent *pevent) {
}

void classSort::handleSwitchEvent(SwitchEvent *pevent) {
  if (pevent->pressed) {
    this->deactivate();
    //Tracks.setAlbum(palbum);
    //Tracks.activate();
  }
}

void classSort::showCurrent() {
  Output.clear();
  switch (current) {
    case Title:
      Output.addText(0, 0, "Albums");
      Output.addText(0, 1, "By title");
      break;
    case Artist:
      Output.addText(0, 0, "Albums");
      Output.addText(0, 1, "By artist");
      break;
  }
}