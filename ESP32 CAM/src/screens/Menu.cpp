#include "Menu.h"

#include <Arduino.h>

#include "../settings.h"
#include "Albums.h"
#include "../data/refresh.h"

extern void restart();

classMenu Menu;

void classMenu::activate() {
  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);

  Output.clear();
  Output.centreText(0, "Plex Radio");
  current = 0;
  showCurrent();
}

void classMenu::deactivate() {
  EventManager.removeListener(this);
}

bool classMenu::onEvent(Event *pevent) {
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

void classMenu::handleEncoderEvent(EncoderEvent *pevent) {
  int previous = current;
  current += pevent->step;
  if (current < 0) current = 0;
  if (current > max) current = max;
  if (previous != current) {
    showCurrent();
  }
}

void classMenu::handleSwitchEvent(SwitchEvent *pevent) {
  if (pevent->pressed == 0) {
    return;
  }

  switch (current) {
    case 0:
      Settings.setSortOrder(Title);
      Albums.activate();
      deactivate();
      break;
    case 1:
      Settings.setSortOrder(Artist);
      Albums.activate();
      deactivate();
      break;
    case 2:
      Refresh::fetchAll();

      // Restart to ensure the database is stable.
      restart();
      
      break;
  }
}

void classMenu::showCurrent() {
  switch (current) {
    case 0:
      Output.centreText(1, "Albums by title");
      break;
    case 1:
      Output.centreText(1, "Albums by artist");
      break;
    case 2:
      Output.centreText(1, "Refresh data");
      break;
  }
}