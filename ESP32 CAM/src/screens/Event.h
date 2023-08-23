#pragma once

class Event {
 protected:
  Event(int type) : type(type) {}

 public:
  int type;
};
