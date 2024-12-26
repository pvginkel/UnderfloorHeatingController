#include "includes.h"

#include "DemoDriverProgram.h"

#include "Controller.h"

LOG_TAG(DemoDriverProgram);

void DemoDriverProgram::doBegin() {
    ESP_LOGI(TAG, "Running demo program");

    const auto stepper = getController()->getStepper();

    stepper->setAcceleration(SET_ACCEL);
    stepper->setSpeedInHz(SET_VELOCITY);
    stepper->setCurrentPosition(0);

    getController()->onStalled([this]() {
        ESP_LOGI(TAG, "Stalled");

        getController()->getStepper()->forceStopAndNewPosition(0);

        _state = STATE_END;
        _waitUntil = millis() + 1000;
    });

    _state = STATE_END;
}

void DemoDriverProgram::reset() {
    _SG_RESULT_low = 65535;
    _SG_RESULT_high = 0;
}

void DemoDriverProgram::doUpdate() {
    const auto currentMillis = millis();
    const auto driver = getController()->getDriver();
    const auto stepper = getController()->getStepper();

    if (_waitUntil) {
        if (currentMillis < _waitUntil) {
            return;
        }
        _waitUntil = 0;
    }

    switch (_state) {
        case STATE_END:
            stepper->moveTo(MOVE_TO_STEP);
            _state = STATE_GOING_UP;
            break;

        case STATE_GOING_UP:
            if (!stepper->isRunning()) {
                stepper->moveTo(0);
                reset();
                _state = STATE_GOING_DOWN;
            }
            break;

        case STATE_GOING_DOWN:
            if (!stepper->isRunning()) {
                reset();
                _state = STATE_END;
            }
            break;
    }

    auto interstepDuration = driver->getInterstepDuration();
    auto stallGuardResult = driver->getStallGuardResult();

    if (interstepDuration && interstepDuration <= CONFIG_DRIVER_TCOOL_THRESHOLD &&
        (stallGuardResult > _SG_RESULT_high || stallGuardResult < _SG_RESULT_low)) {
        _SG_RESULT_high = max(_SG_RESULT_high, stallGuardResult);
        _SG_RESULT_low = min(_SG_RESULT_low, stallGuardResult);

        ESP_LOGI(TAG, "SG_RESULT=%" PRIu16 " high=%" PRIu16 " low=%" PRIu16 " TSTEP=%" PRIu32 " ofs=%d grad=%d psc=%d",
                 stallGuardResult, _SG_RESULT_high, _SG_RESULT_low, interstepDuration, driver->getPwmOffsetAuto(),
                 driver->getPwmGradientAuto(), driver->getPwmScaleSum());
    }
}
