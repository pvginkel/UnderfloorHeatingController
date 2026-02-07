#include "support.h"

#include "Application.h"

#include "MQTTSupport.h"
#include "NVSProperty.h"
#include "driver/i2c.h"
#include "nvs_flash.h"

LOG_TAG(Application);

static NVSPropertyI1 nvs_motor_on("motor_on");
static NVSPropertyU16 nvs_room_on("room_on");

void Application::do_begin() {
    load_state();

    _status_led.begin();

    _status_led.set_color(Colors::Blue);
    _status_led.set_mode(StatusLedMode::Blinking, 400);

    _device.on_motor_on_changed([this](auto state) {
        if (_state.motor_on != state.on) {
            _state.motor_on = state.on;

            ESP_LOGI(TAG, "Motor state changed to %s", state.on ? "ON" : "OFF");

            state_changed();
        }
    });

    _device.on_room_on_changed([this](auto state) {
        if (_state.room_on[state.room] != state.on) {
            _state.room_on[state.room] = state.on;

            ESP_LOGI(TAG, "Room %d state changed to %s", state.room, state.on ? "ON" : "OFF");

            sync_automatic_motor_control();

            state_changed();
        }
    });

    _current_meter.on_current_changed([this](auto current) {
        if (_state.current != current) {
            _state.current = current;

            state_changed();
        }
    });

    get_mqtt_connection().on_publish_discovery([this]() { publish_mqtt_discovery(); });

    get_mqtt_connection().on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });
}

void Application::load_state() {
    ESP_LOGD(TAG, "Loading state");

    nvs_handle_t handle;
    auto err = nvs_open("storage", NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return;
    }
    ESP_ERROR_CHECK(err);

    DEFER(nvs_close(handle));

    _state.motor_on = nvs_motor_on.get(handle, false);

    // We're loading all rooms that would fit in the bitmask. The number of rooms
    // will be corrected when we get the configuration.

    uint16_t room_on = nvs_room_on.get(handle, 0);
    for (int i = 0; i < 16 * 8; i++) {
        _state.room_on.push_back((room_on & (1u << i)) != 0);
    }
}

void Application::save_state() {
    ESP_LOGD(TAG, "Saving state");

    ESP_ASSERT_CHECK(_state.room_on.size() <= 16);

    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));
    DEFER(nvs_close(handle));

    nvs_motor_on.set(handle, _state.motor_on);

    uint16_t room_on = 0;
    for (size_t i = 0; i < _state.room_on.size(); i++) {
        if (_state.room_on[i]) {
            room_on |= 1u << i;
        }
    }
    nvs_room_on.set(handle, room_on);
}

void Application::do_configuration_loaded(cJSON* data) {
    _status_led.set_color(Colors::Green);

    _configuration.load(data);

    _device.set_configuration(&_configuration);

    const auto& rooms = _configuration.get_rooms();
    for (size_t i = 0; i < rooms.size(); i++) {
        _room_index[strformat("room_%s", rooms[i].get_id())] = i;
    }

    // Sync number of rooms in loaded state with the number of rooms in the configuration.
    _state.room_on.resize(rooms.size(), false);
}

void Application::do_ready() {
    _status_led.set_color(Colors::Green);
    _status_led.set_mode(StatusLedMode::Continuous, 3000);

    ESP_LOGI(TAG, "Startup complete");

    ESP_ERROR_CHECK(_device.begin());

    ESP_ERROR_CHECK(_current_meter.begin(CONFIG_DEVICE_CURRENT_METER_REPORT_INTERVAL_MS));

    // Sync loaded state with actual state. Motor must be set on first otherwise the
    // sync motor logic may cause issues.
    _device.set_motor_on(_state.motor_on);

    for (size_t i = 0; i < _state.room_on.size(); i++) {
        _device.set_room_on(i, _state.room_on[i]);
    }
}

void Application::do_process() { _status_led.process(); }

void Application::state_changed() {
    save_state();

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

    if (_state.motor_on != any_room_on) {
        ESP_LOGI(TAG, "Automatic motor control: setting motor to %s", any_room_on ? "ON" : "OFF");

        _device.set_motor_on(any_room_on);
    }
}
