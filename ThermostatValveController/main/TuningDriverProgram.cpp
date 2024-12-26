#include "includes.h"

#include "TuningDriverProgram.h"

#include "Controller.h"
#include "DemoDriverProgram.h"

LOG_TAG(TuningDriverProgram);

void TuningDriverProgram::doBegin() {
    ESP_LOGI(TAG, "Running tuning program");

    const auto stepper = getController()->getStepper();

    stepper->setAcceleration(5000);
    stepper->setSpeedInHz(2000);
    stepper->setCurrentPosition(0);

    stepper->moveTo(CONFIG_DRIVER_MICROSTEPS * 10);
    _state = STATE_BEFORE_PAUSE;
}

void TuningDriverProgram::doUpdate() {
    const auto driver = getController()->getDriver();
    const auto stepper = getController()->getStepper();

    const auto currentMillis = millis();

    switch (_state) {
        case STATE_BEFORE_PAUSE:
            if (!stepper->isRunning()) {
                stepper->setSpeedInHz(0);
                _intervalStart = currentMillis;
                _state = STATE_WAITING;
            }
            break;

        case STATE_WAITING:
            if (currentMillis - _intervalStart > WAIT_MS) {
                stepper->setSpeedInHz(2000);
                stepper->moveTo(999999);
                _intervalStart = currentMillis;
                _state = STATE_CALIBRATING;
            }
            break;

        case STATE_CALIBRATING:
            if (currentMillis - _intervalStart > CALIBRATING_MS) {
                getController()->setProgram(nullptr);
            }
            break;
    }

    if (currentMillis - _lastPrint >= PRINT_INTERVAL_MS) {
        _lastPrint = currentMillis;

        const char* state;

        switch (_state) {
            case STATE_BEFORE_PAUSE:
                state = "STATE_BEFORE_PAUSE";
                break;
            case STATE_WAITING:
                state = "STATE_WAITING";
                break;
            case STATE_CALIBRATING:
            default:
                state = "STATE_CALIBRATING";
                break;
        }

        ESP_LOGI(TAG, "ofs=%d grad=%d psc=%d state=%s", driver->getPwmOffsetAuto(), driver->getPwmGradientAuto(),
                 driver->getPwmScaleSum(), state);
    }
}
