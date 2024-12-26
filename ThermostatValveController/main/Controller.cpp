#include "includes.h"

#include "Controller.h"

LOG_TAG(Controller);

void IRAM_ATTR Controller::driverDiagRaised() { _hasStalled = true; }

volatile bool Controller::_hasStalled = false;

Controller::Controller(HardwareSerial& serial) { _driver.setup(serial); }

#define STEPPER

void Controller::begin() {
    pinMode(CONFIG_PIN_ENABLE, OUTPUT);
    pinMode(CONFIG_PIN_DIAG, INPUT);

    _engine.init();
    _stepper = _engine.stepperConnectToPin(CONFIG_PIN_STEP);
    _stepper->setDirectionPin(CONFIG_PIN_DIR);
    _stepper->setEnablePin(CONFIG_PIN_ENABLE);
    _stepper->setAutoEnable(true);
    _stepper->setCurrentPosition(0);

    attachInterrupt(digitalPinToInterrupt(CONFIG_PIN_DIAG), driverDiagRaised, RISING);

    _driver.enable();

    _driver.setRMSCurrent(CONFIG_DRIVER_CURRENT, CONFIG_DRIVER_R_SENSE);
    _driver.setMicrostepsPerStep(CONFIG_DRIVER_MICROSTEPS);

    // A stall is signaled with SG_RESULT <= SGTHRS * 2.
    _driver.setStallGuardThreshold(CONFIG_DRIVER_SG_THRESHOLD / 2);

    // The stall output signal become enabled when exceeding this velocity.
    // TCOOLTHRS >= TSTEP >= TPWMTHRS.
    _driver.setCoolStepDurationThreshold(CONFIG_DRIVER_TCOOL_THRESHOLD);

    _driver.enableAutomaticCurrentScaling();
    _driver.enableAutomaticGradientAdaptation();

    ESP_LOGI(TAG, "Checkinging whether the TMC2209 is communicating");

    ESP_ERROR_ASSERT(_driver.isSetupAndCommunicating());
}

void Controller::update() {
    if (_hasStalled) {
        _hasStalled = false;

        _stalled.call();
    }

    if (_program) {
        _program->update();
    }
}

void Controller::setProgram(DriverProgram* program) {
    const auto oldProgram = _program;

    _program = program;
    _stalled.clear();

    if (oldProgram) {
        oldProgram->end();
    }

    // Protect against recursive calls.

    if (_program == program && program) {
        program->begin(this);
    }
}
