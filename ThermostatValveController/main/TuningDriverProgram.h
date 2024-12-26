#pragma once

#include "DriverProgram.h"

class TuningDriverProgram : public DriverProgram {
    static constexpr auto WAIT_MS = 130UL;
    static constexpr auto CALIBRATING_MS = 2000UL;
    static constexpr auto PRINT_INTERVAL_MS = 50UL;

    enum STATE {
        STATE_BEFORE_PAUSE,
        STATE_WAITING,
        STATE_CALIBRATING,
    };

    STATE _state;
    unsigned long _intervalStart;
    unsigned long _lastPrint;

protected:
    virtual void doBegin();
    virtual void doUpdate();
};
