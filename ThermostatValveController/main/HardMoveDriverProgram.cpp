#include "includes.h"

#include "HardMoveDriverProgram.h"

#include "Controller.h"

LOG_TAG(HardMoveDriverProgram);

void HardMoveDriverProgram::doBegin() {
    ESP_LOGI(TAG, "Running hard move program");

    const auto stepper = getController()->getStepper();

    stepper->setAcceleration(SET_ACCEL);
    stepper->setSpeedInHz(SET_VELOCITY);
    stepper->forceStopAndNewPosition(0);
}

void HardMoveDriverProgram::reset() {
    _SG_RESULT_low = 65535;
    _SG_RESULT_high = 0;
}

void HardMoveDriverProgram::doUpdate() {
    const auto currentMillis = millis();
    const auto driver = getController()->getDriver();
    const auto stepper = getController()->getStepper();

    if (_waitUntil) {
        if (currentMillis < _waitUntil) {
            return;
        }
        _waitUntil = 0;
    }

    if (!stepper->isRunning()) {
        const auto rand = esp_random();
        const auto move = (uint32_t)(rand % (MOVE_MAX * 2)) - MOVE_MAX;

        stepper->move(move);
        reset();
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
