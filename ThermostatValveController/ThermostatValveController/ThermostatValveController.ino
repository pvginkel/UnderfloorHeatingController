/*
#include <Arduino.h>

#define STEP_PIN 3
#define DIR_PIN 4

void setup() {
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);
	digitalWrite(DIR_PIN, HIGH);  // Set direction
}

void loop() {
	digitalWrite(STEP_PIN, HIGH);
	delayMicroseconds(1000);  // Pulse width
	digitalWrite(STEP_PIN, LOW);
	delayMicroseconds(1000);
}
*/

#include "Controller.h"

// Not all pins on the Leonardo and Micro boards support change interrupts,
// so only the following can be used for RX: 8, 9, 10, 11, 14 (MISO),
// 15 (SCK), 16 (MOSI).

constexpr uint8_t DRIVER_ENABLE = 2;
constexpr uint8_t DRIVER_STEP = 3;
constexpr uint8_t DRIVER_DIR = 4;
constexpr uint8_t DRIVER_INDEX = 5;
constexpr uint8_t DRIVER_DIAG = 6;

Controller controller(Serial1, DRIVER_ENABLE, DRIVER_INDEX, DRIVER_DIAG);


void setup() {
	controller.begin();
}

void loop() {
	controller.update();
}
