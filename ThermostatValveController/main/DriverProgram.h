#pragma once

#include "FastAccelStepper.h"
#include "TMC2209.h"

class Controller;

class DriverProgram {
    Controller* _controller;
    Callback<void> _finished;

public:
    virtual ~DriverProgram() {}

    void begin(Controller* controller) {
        _controller = controller;

        doBegin();
    }

    void update() { doUpdate(); }
    
    void end() {
        doEnd();

        _finished.call();
    }

    Controller* getController() { return _controller; }
    void onFinished(function<void(void)> func) { _finished.add(func); }

protected:
    virtual void doBegin() {}
    virtual void doUpdate() {}
    virtual void doEnd() {}
};
