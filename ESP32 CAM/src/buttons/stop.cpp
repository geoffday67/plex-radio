#include "stop.h"
#include "pins.h"

namespace StopButton {

Button button(STOP_LIGHT);

void setState(Button::State state) {
  button.setState(state);
}

void begin() {
  button.begin(Button::State::Off);
}

}  // namespace StopButton