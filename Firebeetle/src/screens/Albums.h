#pragma once

#include "Screen.h"
#include "EventManager.h"
#include "EventReceiver.h"

class classAlbums: public Screen, EventReceiver {
public:
    virtual void activate();
    virtual void deactivate();
    virtual bool onEvent(Event* pevent);

private:
};

extern classAlbums Albums;
