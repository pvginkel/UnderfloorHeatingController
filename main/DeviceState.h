#pragma once

#include <vector>

struct DeviceState {
    bool motor_on{};
    std::vector<bool> room_on;
};
