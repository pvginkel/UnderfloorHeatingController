#include "support.h"

#include "DeviceConfiguration.h"

#include <set>

#include "esp_mac.h"

LOG_TAG(DeviceConfiguration);

esp_err_t DeviceConfiguration::load(cJSON* data) {
    auto automatic_motor_control = cJSON_GetObjectItemCaseSensitive(data, "automaticMotorControl");
    if (automatic_motor_control) {
        if (!cJSON_IsBool(automatic_motor_control)) {
            ESP_LOGE(TAG, "automaticMotorControl must be a boolean");
            return ESP_ERR_INVALID_ARG;
        }

        _automatic_motor_control = cJSON_IsTrue(automatic_motor_control);
    }

    auto rooms = cJSON_GetObjectItemCaseSensitive(data, "rooms");
    if (!cJSON_IsArray(rooms)) {
        ESP_LOGE(TAG, "Cannot get rooms");
        return ESP_ERR_INVALID_ARG;
    }

    set<int> assigned_pins;

    cJSON* room = nullptr;
    cJSON_ArrayForEach(room, rooms) {
        if (!cJSON_IsObject(room)) {
            ESP_LOGE(TAG, "Room must be an object");
            return ESP_ERR_INVALID_ARG;
        }

        auto room_id = cJSON_GetObjectItemCaseSensitive(room, "id");
        if (!cJSON_IsString(room_id) || !room_id->valuestring) {
            ESP_LOGE(TAG, "Room ID must be a string");
            return ESP_ERR_INVALID_ARG;
        }

        auto room_name = cJSON_GetObjectItemCaseSensitive(room, "name");
        if (!cJSON_IsString(room_name) || !room_name->valuestring) {
            ESP_LOGE(TAG, "Room name must be a string");
            return ESP_ERR_INVALID_ARG;
        }

        auto room_pins = cJSON_GetObjectItemCaseSensitive(room, "pins");
        if (!cJSON_IsArray(room_pins)) {
            ESP_LOGE(TAG, "Room pins must be an array");
            return ESP_ERR_INVALID_ARG;
        }

        vector<int> room_pin_vector;
        cJSON* room_pin = nullptr;
        cJSON_ArrayForEach(room_pin, room_pins) {
            if (!cJSON_IsNumber(room_pin)) {
                ESP_LOGE(TAG, "Room pin must be a number");
                return ESP_ERR_INVALID_ARG;
            }

            if (room_pin->valueint < 1 || room_pin->valueint > 12) {
                ESP_LOGE(TAG, "Room pin must be between 1 and 12");
                return ESP_ERR_INVALID_ARG;
            }

            if (assigned_pins.contains(room_pin->valueint)) {
                ESP_LOGE(TAG, "Room pin number %d has already been assigned", room_pin->valueint);
                return ESP_ERR_INVALID_ARG;
            }

            assigned_pins.insert(room_pin->valueint);

            room_pin_vector.push_back(room_pin->valueint);
        }

        _rooms.push_back(RoomConfiguration(room_id->valuestring, room_name->valuestring, room_pin_vector));

        ESP_LOGI(TAG, "Room ID %s, name %s, pin count %d", room_id->valuestring, room_name->valuestring,
                 room_pin_vector.size());
    };

    return ERR_OK;
}
