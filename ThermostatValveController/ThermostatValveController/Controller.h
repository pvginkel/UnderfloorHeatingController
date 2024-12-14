#pragma once

#include "Arduino.h"
#include <SoftwareSerial.h>
#include <TMC2209.h>

class Controller {
	HardwareSerial& _serial;
	TMC2209 _driver;
	uint8_t _driver_index;
	uint8_t _driver_diag;

public:
	Controller(HardwareSerial& serial, uint8_t driver_enable, uint8_t driver_index, uint8_t driver_diag);

	void begin();
	void update();
};
