#pragma once

#include "ACS725.h"
#include "Callback.h"
#include "DeviceConfiguration.h"

class UFHController {
    DeviceConfiguration* _configuration{};

public:
    esp_err_t begin();
    void set_configuration(DeviceConfiguration* configuration) { _configuration = configuration; }
    void set_motor_on(bool enabled);
    void set_room_on(int index, bool enabled);
};
