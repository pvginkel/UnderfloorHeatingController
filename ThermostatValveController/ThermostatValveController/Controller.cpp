#include "Controller.h"

Controller::Controller(HardwareSerial& serial, uint8_t driver_enable, uint8_t driver_index,
	uint8_t driver_diag) : _serial(serial),
	_driver_index(driver_index), _driver_diag(driver_diag) {
    _driver.setHardwareEnablePin(driver_enable);
}

void Controller::begin() {
    _driver.setup(_serial);
    _driver.enable();
}

void Controller::update() {
    if (_driver.isSetupAndCommunicating())
    {
        Serial.println("SUCCESS! Try turning driver power off to see what happens.");
    }
    else if (_driver.isCommunicatingButNotSetup())
    {
        Serial.println("Stepper driver is communicating but not setup!");
        Serial.println("Running setup again...");
        // _driver.setup(serial_stream);
    }
    else
    {
        Serial.println("FAILURE! Try turning driver power on to see what happens.");
    }
    Serial.println();
    delay(5000);
}
