#pragma once

#include "ApplicationBase.h"
#include "DeviceConfiguration.h"
#include "UFHController.h"
#include "WS2812StatusLed.h"

class Application : public ApplicationBase {
    struct DeviceState {
        float current;
        bool motor_on;
        vector<bool> room_on;
    };

    DeviceState _state{};
    UFHController _device;
    ACS725 _current_meter;
    map<string, size_t> _room_index;
    DeviceConfiguration _configuration;
    WS2812StatusLed _status_led;

public:
    Application() : _current_meter(&get_queue()) {}

protected:
    void do_begin() override;
    void do_ready() override;
    void do_configuration_loaded(cJSON* data) override;
    void do_process() override;

private:
    void state_changed();
    void publish_mqtt_discovery();
    void sync_automatic_motor_control();
    void load_state();
    void save_state();
};
