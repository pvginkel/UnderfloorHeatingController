#pragma once

#include "ACS725.h"
#include "Callback.h"
#include "DeviceConfiguration.h"

struct UHFMotorState {
    bool on{};
};

struct UHFRoomState {
    int room{};
    bool on{};
};

class UFHController {
    DeviceConfiguration* _configuration{};
    Callback<UHFMotorState> _motor_on_changed;
    Callback<UHFRoomState> _room_on_changed;

public:
    esp_err_t begin();
    void set_configuration(DeviceConfiguration* configuration) { _configuration = configuration; }
    void set_motor_on(bool enabled);
    void set_room_on(int index, bool enabled);
    void on_motor_on_changed(std::function<void(UHFMotorState)> func) { _motor_on_changed.add(func); }
    void on_room_on_changed(std::function<void(UHFRoomState)> func) { _room_on_changed.add(func); }
};
