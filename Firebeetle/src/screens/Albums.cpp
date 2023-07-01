#include "Albums.h"

classAlbums Albums;

void classAlbums::activate() {
  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);

  Output.addText(0, 0, "Albums");
}

/*
How to represent the album data?
"album" object.
Data source to get next/previous - uses database
*/

void classAlbums::deactivate() {
  EventManager.removeListener(this);
}

bool classAlbums::onEvent(Event* pevent) {
  /*switch (pevent->type) {
      case EVENT_ENCODER:
          handleEncoderEvent((EncoderEvent*) pevent);
          break;
      case EVENT_SWITCH:
          handleSwitchEvent((SwitchEvent*) pevent);
          break;
  }*/
  return true;
}