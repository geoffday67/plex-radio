#include "play.h"
#include "pins.h"

namespace PlayButton {

Button button(PLAY_LIGHT);

void setState(Button::State state) {
  button.setState(state);
}

void begin() {
  button.begin(Button::State::Off);
}

}  // namespace PlayButton