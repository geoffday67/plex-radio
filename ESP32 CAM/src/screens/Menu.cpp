#include "Menu.h"

#include <Arduino.h>

#include "../settings.h"
#include "Albums.h"
#include "../data/refresh.h"

classMenu Menu;

void classMenu::activate() {
  EventManager.addListener(EVENT_ENCODER, this);
  EventManager.addListener(EVENT_SWITCH, this);

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
      Settings.setSortOrder(Artist);
      Albums.activate();
      deactivate();
      break;
    case 1:
      Settings.setSortOrder(Title);
      Albums.activate();
      deactivate();
      break;
    case 2:
      Refresh::fetchAll();

      // Restart to ensure the database is stable.
      ESP.restart();
      
      break;
  }
}

void classMenu::showCurrent() {
  switch (current) {
    case 0:
      Output.setLine(0, "Albums by artist");
      break;
    case 1:
      Output.setLine(0, "Albums by title");
      break;
    case 2:
      Output.setLine(0, "Refresh data");
      break;
  }
  if (current > 0) {
    Output.addChar(0, 1, LEFT_ARROW);
  } else {
    Output.addChar(0, 1, ' ');
  }
  if (current < max) {
    Output.addChar(15, 1, RIGHT_ARROW);
  } else {
    Output.addChar(15, 1, ' ');
  }
}