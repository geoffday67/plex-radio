namespace PlayButton {

enum State {
  Off,
  Flashing,
  On
};

void begin(State initialState = Off);
void setState(State);

}  // namespace PlayButton