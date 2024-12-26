#pragma once

#include "DriverProgram.h"

class HardMoveDriverProgram : public DriverProgram {
    static constexpr auto SET_VELOCITY = 30000;
    static constexpr auto MOVE_MAX = 5000;
    static constexpr auto SET_ACCEL = 300000;

    uint16_t _SG_RESULT_low;
    uint16_t _SG_RESULT_high;
    unsigned long _waitUntil{};

    void reset();

protected:
    virtual void doBegin();
    virtual void doUpdate();
};
