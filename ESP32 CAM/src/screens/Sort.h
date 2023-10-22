#pragma once

#include "EventManager.h"
#include "EventReceiver.h"
#include "Screen.h"
#include "utils.h"

class classSort : public Screen, EventReceiver {
 private:
  SortOrder current;
  void showCurrent();
  void handleEncoderEvent(EncoderEvent*);
  void handleSwitchEvent(SwitchEvent*);

 public:
  classSort();
  virtual void activate();
  virtual void deactivate();
  virtual bool onEvent(Event* pevent);
};

extern classSort Sort;