#include "support.h"

#include "Application.h"

#include "MQTTSupport.h"
#include "driver/i2c.h"
#include "nvs_flash.h"

LOG_TAG(Application);

void Application::do_begin() {
    _status_led.begin();

    _status_led.set_color(Colors::Blue);
    _status_led.set_mode(StatusLedMode::Blinking, 400);

    get_mqtt_connection().on_publish_discovery([this]() { publish_mqtt_discovery(); });

    get_mqtt_connection().on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });
}

void Application::do_configuration_loaded(cJSON* data) {
    _status_led.set_color(Colors::Green);

    _configuration.load(data);

    _device.set_configuration(&_configuration);

    const auto& rooms = _configuration.get_rooms();
    for (size_t i = 0; i < rooms.size(); i++) {
        _state.room_on.push_back(false);
        _room_index[strformat("room_%s", rooms[i].get_id())] = i;
    }
}

void Application::do_ready() {
    _status_led.set_color(Colors::Green);
    _status_led.set_mode(StatusLedMode::Continuous, 3000);

    ESP_LOGI(TAG, "Startup complete");

    ESP_ERROR_CHECK(_device.begin());

    ESP_ERROR_CHECK(_current_meter.begin(CONFIG_DEVICE_CURRENT_METER_REPORT_INTERVAL_MS));

    _current_meter.on_current_changed([this](auto current) {
        if (_state.current != current) {
            _state.current = current;

            state_changed();
        }
    });
}

void Application::do_process() { _status_led.process(); }

void Application::state_changed() {
    if (!get_mqtt_connection().is_connected()) {
        return;
    }

    // Signal activity.
    if (!_status_led.is_active()) {
        _status_led.set_mode(StatusLedMode::Continuous, 1);
    }

    const auto json = cJSON_CreateObject();
    ESP_ASSERT_CHECK(json);
    DEFER(cJSON_Delete(json));

    cJSON_AddNumberToObject(json, "current", _state.current);
    cJSON_AddStringToObject(json, "motor_on", print_switch_state(_state.motor_on ? SwitchState::ON : SwitchState::OFF));

    const auto& rooms = _configuration.get_rooms();
    for (auto i = 0; i < rooms.size(); i++) {
        cJSON_AddStringToObject(json, strformat("room_%s_on", rooms[i].get_id()).c_str(),
                                print_switch_state(_state.room_on[i] ? SwitchState::ON : SwitchState::OFF));
    }

    get_mqtt_connection().send_state(json);
}

void Application::publish_mqtt_discovery() {
    get_mqtt_connection().publish_button_discovery(
        {
            .name = "Identify",
            .object_id = "identify",
            .entity_category = "config",
            .device_class = "identify",
        },
        []() { ESP_LOGI(TAG, "Requested identification"); });

    get_mqtt_connection().publish_button_discovery(
        {
            .name = "Restart",
            .object_id = "restart",
            .entity_category = "config",
            .device_class = "restart",
        },
        []() {
            ESP_LOGI(TAG, "Requested restart");

            esp_restart();
        });

    get_mqtt_connection().publish_sensor_discovery(
        {
            .name = "Current",
            .object_id = "current",
            .device_class = "current",
        },
        {
            .state_class = "measurement",
            .unit_of_measurement = "A",
            .value_template = "{{ value_json.current }}",
        });

    get_mqtt_connection().publish_switch_discovery(
        {
            .name = "Motor",
            .object_id = "motor",
        },
        {
            .value_template = "{{ value_json.motor_on }}",
        },
        [this](auto state) {
            ESP_LOGI(TAG, "Requested motor switch change");

            _device.set_motor_on(state);

            _state.motor_on = state;

            state_changed();
        });

    for (size_t room_index = 0; room_index < _configuration.get_rooms().size(); room_index++) {
        const auto& room = _configuration.get_rooms()[room_index];

        get_mqtt_connection().publish_switch_discovery(
            {
                .name = room.get_name().c_str(),
                .object_id = strformat("room_%s", room.get_id()).c_str(),
            },
            {
                .value_template = strformat("{{ value_json.room_%s_on }}", room.get_id()).c_str(),
            },
            [this, room_index](auto state) {
                ESP_LOGI(TAG, "Requested room switch change for room %d", room_index);

                _device.set_room_on(room_index, state);

                _state.room_on[room_index] = state;

                sync_automatic_motor_control();

                state_changed();
            });
    }
}

void Application::sync_automatic_motor_control() {
    if (!_configuration.get_automatic_motor_control()) {
        return;
    }

    bool any_room_on = false;
    for (const auto& room_on : _state.room_on) {
        if (room_on) {
            any_room_on = true;
            break;
        }
    }

    ESP_LOGI(TAG, "Automatic motor control: setting motor to %s", any_room_on ? "ON" : "OFF");

    _device.set_motor_on(any_room_on);
}
