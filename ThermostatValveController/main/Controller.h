#pragma once

#include "DriverProgram.h"
#include "FastAccelStepper.h"
#include "TMC2209.h"

class Controller {
    static void IRAM_ATTR driverDiagRaised();

    static volatile bool _hasStalled;

    TMC2209 _driver;
    FastAccelStepperEngine _engine;
    FastAccelStepper* _stepper;
    Callback<void> _stalled;
    DriverProgram* _program{};

public:
    Controller(HardwareSerial& serial);

    void begin();
    void update();
    void onStalled(function<void(void)> func) { _stalled.add(func); }
    void setProgram(DriverProgram* program);
    FastAccelStepper* getStepper() { return _stepper; }
    TMC2209* getDriver() { return &_driver; }
};
