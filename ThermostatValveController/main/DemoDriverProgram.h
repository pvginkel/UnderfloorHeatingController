#pragma once

#include "DriverProgram.h"

class DemoDriverProgram : public DriverProgram {
    static constexpr auto SET_VELOCITY = 10000;
    static constexpr auto MOVE_TO_STEP = SET_VELOCITY * 4;
    static constexpr auto SET_ACCEL = 5000;

    enum STATE {
        STATE_END,
        STATE_GOING_UP,
        STATE_GOING_DOWN,
    };

    STATE _state;
    uint16_t _SG_RESULT_low;
    uint16_t _SG_RESULT_high;
    unsigned long _waitUntil{};

    void reset();

protected:
    virtual void doBegin();
    virtual void doUpdate();
};
