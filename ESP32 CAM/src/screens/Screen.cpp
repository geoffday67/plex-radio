#include "Screen.h"
#include "Output.h"

Screen::Screen () {
}

Screen::~Screen() {}

void Screen::activate() {
    active = true;
    onActivate();
}

void Screen::onActivate() {
}

void Screen::onDeactivate() {
}

void Screen::deactivate() {
    active = false;
    onDeactivate();
}

void Screen::showWaiting() {
}

void Screen::showSuccess() {
}

void Screen::showFailure() {
}
