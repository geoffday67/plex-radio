#pragma once

class Button {
 public:
  enum State {
    Off,
    Flashing,
    On
  };

  Button(int lightPin);
  void begin(State);
  void setState(State);

 private:
  int lightPin;
  State state;
  bool showing;
  void show(bool);
  static void buttonTask(void*);
};