#pragma once

#include <vector>

class RoomConfiguration {
    string _id;
    string _name;
    vector<int> _pins;

public:
    RoomConfiguration(const string& id, const string& name, const vector<int>& pins)
        : _id(id), _name(name), _pins(pins) {}

    const string& get_id() const { return _id; }
    const string& get_name() const { return _name; }
    const vector<int>& get_pins() const { return _pins; }
};

class DeviceConfiguration {
    static constexpr auto DEFAULT_ENABLE_OTA = true;

    bool _automatic_motor_control{};
    vector<RoomConfiguration> _rooms;

public:
    DeviceConfiguration() = default;
    DeviceConfiguration(const DeviceConfiguration&) = delete;
    DeviceConfiguration& operator=(const DeviceConfiguration&) = delete;
    DeviceConfiguration(DeviceConfiguration&&) = delete;
    DeviceConfiguration& operator=(DeviceConfiguration&&) = delete;

    esp_err_t load(cJSON* data);

    bool get_automatic_motor_control() const { return _automatic_motor_control; }
    const vector<RoomConfiguration>& get_rooms() const { return _rooms; }
};
