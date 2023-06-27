#include "Screen.h"
#include "Output.h"
#include "wait.xbm"
#include "cross.xbm"
#include "tick.xbm"

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
    Output.clear();
    Output.drawBitmap(wait_bits);
    Output.flush();
}

void Screen::showSuccess() {
    Output.clear();
    Output.drawBitmap(tick_bits);
    Output.flush();
}

void Screen::showFailure() {
    Output.clear();
    Output.drawBitmap(cross_bits);
    Output.flush();
}
