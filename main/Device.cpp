#include "support.h"

#include "Device.h"

#include "MQTTSupport.h"
#include "NVSProperty.h"

LOG_TAG(Device);

Device::Device(Queue* queue, MQTTConnection& mqtt_connection)
    : _mqtt_connection(mqtt_connection), _current_meter(queue) {}

void Device::begin() {
    load_state();

    ESP_ERROR_CHECK(_device.begin());

    ESP_ERROR_CHECK(_current_meter.begin(CONFIG_DEVICE_CURRENT_METER_REPORT_INTERVAL_MS));

    _mqtt_connection.on_publish_discovery([this]() { publish_mqtt_discovery(); });
    _mqtt_connection.on_set_message([this](auto data) { handle_set_message(data.topic, data.data); });

    _mqtt_connection.on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });

    _current_meter.on_current_changed([this](auto current) {
        if (_state.current != current) {
            _state.current = current;

            state_changed();
        }
    });
}

void Device::set_configuration(DeviceConfiguration* configuration) {
    _configuration = configuration;

    _device.set_configuration(configuration);

    const auto& rooms = configuration->get_rooms();
    for (size_t i = 0; i < rooms.size(); i++) {
        _state.room_on.push_back(false);
        _room_index[strformat("room_%s", rooms[i].get_id().c_str())] = i;
    }
}

void Device::state_changed() {
    if (!_mqtt_connection.is_connected()) {
        return;
    }

    cJSON_Data data = {cJSON_CreateObject()};

    cJSON_AddNumberToObject(*data, "current", _state.current);
    cJSON_AddStringToObject(*data, "motor_on",
                            print_switch_state(_state.motor_on ? SwitchState::ON : SwitchState::OFF));

    const auto& rooms = _configuration->get_rooms();
    for (auto i = 0; i < rooms.size(); i++) {
        cJSON_AddStringToObject(*data, strformat("room_%s_on", rooms[i].get_id().c_str()).c_str(),
                                print_switch_state(_state.room_on[i] ? SwitchState::ON : SwitchState::OFF));
    }

    _activity.call();

    _mqtt_connection.send_state(*data);
}

void Device::load_state() {
    // The device doesn't have any structured state.
}

void Device::save_state() {
    // The device doesn't have any structured state.
}

void Device::publish_mqtt_discovery() {
    _mqtt_connection.publish_button_discovery({
        .name = "Identify",
        .object_id = "identify",
        .entity_category = "config",
        .device_class = "identify",
    });
    _mqtt_connection.publish_button_discovery({
        .name = "Restart",
        .object_id = "restart",
        .entity_category = "config",
        .device_class = "restart",
    });
    _mqtt_connection.publish_sensor_discovery(
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

    _mqtt_connection.publish_switch_discovery(
        {
            .name = "Motor",
            .object_id = "motor",
        },
        {
            .value_template = "{{ value_json.motor_on }}",
        });

    for (const auto& room : _configuration->get_rooms()) {
        _mqtt_connection.publish_switch_discovery(
            {
                .name = room.get_name().c_str(),
                .object_id = strformat("room_%s", room.get_id().c_str()).c_str(),
            },
            {
                .value_template = strformat("{{ value_json.room_%s_on }}", room.get_id().c_str()).c_str(),
            });
    }
}

void Device::handle_set_message(const string& topic, const string& data) {
    if (topic == "identify") {
        ESP_LOGI(TAG, "Requested identification");
    } else if (topic == "restart") {
        ESP_LOGI(TAG, "Requested restart");

        esp_restart();
    } else if (topic == "motor") {
        ESP_LOGI(TAG, "Requested motor switch change");

        auto switch_state = parse_switch_state(data.c_str());
        if (switch_state != SwitchState::UNKNOWN) {
            _device.set_motor_on(switch_state == SwitchState::ON);

            _state.motor_on = switch_state == SwitchState::ON;

            state_changed();
        }
    } else {
        auto it = _room_index.find(topic);
        if (it != _room_index.end()) {
            ESP_LOGI(TAG, "Requested room switch change for room %d", it->second);

            auto switch_state = parse_switch_state(data.c_str());
            if (switch_state != SwitchState::UNKNOWN) {
                _device.set_room_on(it->second, switch_state == SwitchState::ON);

                _state.room_on[it->second] = switch_state == SwitchState::ON;

                state_changed();
            }
        } else {
            ESP_LOGE(TAG, "Unknown topic %s", topic.c_str());
        }
    }
}
