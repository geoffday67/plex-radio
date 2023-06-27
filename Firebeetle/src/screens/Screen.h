#pragma once

#include "Output.h"

class Screen {
protected:
    Screen ();
    virtual ~Screen();

    bool active;
    void showWaiting();
    void showSuccess();
    void showFailure();
    virtual void onActivate();
    virtual void onDeactivate();

public:
    virtual void activate();
    virtual void deactivate();
};
