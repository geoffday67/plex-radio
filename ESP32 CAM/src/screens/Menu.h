#pragma once

#include "EventManager.h"
#include "EventReceiver.h"
#include "Screen.h"

class classMenu : public Screen, EventReceiver {
 private:
  const int max = 2;
  int current;
  void showCurrent();
  void handleEncoderEvent(EncoderEvent*);
  void handleSwitchEvent(SwitchEvent*);

 public:
  virtual void activate();
  virtual void deactivate();
  virtual bool onEvent(Event* pevent);
};

extern classMenu Menu;
