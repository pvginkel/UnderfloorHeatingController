#pragma once

#include <map>
#include <vector>

#include "DeviceConfiguration.h"
#include "MQTTConnection.h"
#include "UFHController.h"

class Device {
    struct DeviceState {
        float current;
        bool motor_on;
        vector<bool> room_on;
    };

    MQTTConnection& _mqtt_connection;
    DeviceState _state{};
    UFHController _device;
    ACS725 _current_meter;
    DeviceConfiguration* _configuration;
    map<string, size_t> _room_index;
    Callback<void> _activity;

public:
    Device(Queue* queue, MQTTConnection& mqtt_connection);

    void begin();
    void set_configuration(DeviceConfiguration* configuration);
    void on_activity(function<void()> func) { _activity.add(func); }

private:
    void state_changed();
    void load_state();
    void save_state();
    void publish_mqtt_discovery();
    void handle_set_message(const string& topic, const string& data);
};
