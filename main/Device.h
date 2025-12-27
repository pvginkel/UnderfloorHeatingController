#pragma once

#include "DeviceState.h"
#include "MQTTConnection.h"
#include "UFHController.h"

class Device {
    MQTTConnection& _mqtt_connection;
    DeviceState _state;
    UFHController _device;

public:
    Device(MQTTConnection& mqtt_connection);

    void begin();
    void process();

private:
    void state_changed();
    void load_state();
    void save_state();
};
