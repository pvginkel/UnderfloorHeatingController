#include "support.h"

#include "DeviceConfiguration.h"

#include <set>

#include "esp_mac.h"

LOG_TAG(DeviceConfiguration);

DeviceConfiguration::DeviceConfiguration() : _enable_ota(DEFAULT_ENABLE_OTA) {
    uint8_t mac[6];

    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));

    auto formattedMac = strformat("%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    _endpoint = strformat(CONFIG_DEVICE_CONFIG_ENDPOINT, formattedMac.c_str());
}

esp_err_t DeviceConfiguration::load() {
    esp_http_client_config_t config = {
        .url = get_endpoint().c_str(),
        .timeout_ms = CONFIG_OTA_RECV_TIMEOUT,
    };

    ESP_LOGI(TAG, "Getting device configuration from %s", config.url);

    string json;
    auto err = esp_http_download_string(config, json, 128 * 1024);
    if (err != ESP_OK) {
        return err;
    }

    cJSON_Data data = {cJSON_Parse(json.c_str())};
    if (!*data) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        err = ESP_ERR_INVALID_ARG;
        return err;
    }

    auto device_name_item = cJSON_GetObjectItemCaseSensitive(*data, "deviceName");
    if (!cJSON_IsString(device_name_item) || !device_name_item->valuestring) {
        ESP_LOGE(TAG, "Cannot get deviceName property");
        return ESP_ERR_INVALID_ARG;
    }

    _device_name = device_name_item->valuestring;

    ESP_LOGI(TAG, "Device name: %s", _device_name.c_str());

    auto device_entity_id_item = cJSON_GetObjectItemCaseSensitive(*data, "deviceEntityId");
    if (!cJSON_IsString(device_entity_id_item) || !device_entity_id_item->valuestring) {
        ESP_LOGE(TAG, "Cannot get deviceEntityIdItem property");
        return ESP_ERR_INVALID_ARG;
    }

    _device_entity_id = device_entity_id_item->valuestring;

    ESP_LOGI(TAG, "Device entity ID: %s", _device_entity_id.c_str());

    auto enable_ota_item = cJSON_GetObjectItemCaseSensitive(*data, "enableOTA");
    if (enable_ota_item != nullptr) {
        if (!cJSON_IsBool(enable_ota_item)) {
            ESP_LOGE(TAG, "Cannot get enableOTAItem property");
            return ESP_ERR_INVALID_ARG;
        }

        _enable_ota = cJSON_IsTrue(enable_ota_item);
    }

    ESP_LOGI(TAG, "Enable OTA: %s", _enable_ota ? "yes" : "no");

    auto mqtt_item = cJSON_GetObjectItemCaseSensitive(*data, "mqtt");
    if (!cJSON_IsObject(mqtt_item)) {
        ESP_LOGE(TAG, "Cannot get mqtt property");
        return ESP_ERR_INVALID_ARG;
    }

    auto mqtt_endpoint_item = cJSON_GetObjectItemCaseSensitive(mqtt_item, "endpoint");
    if (!cJSON_IsString(mqtt_endpoint_item) || !mqtt_endpoint_item->valuestring) {
        ESP_LOGE(TAG, "Cannot get mqtt.endpoint property");
        return ESP_ERR_INVALID_ARG;
    }

    _mqtt_endpoint = mqtt_endpoint_item->valuestring;

    ESP_LOGI(TAG, "MQTT endpoint: %s", _mqtt_endpoint.c_str());

    auto mqtt_username_item = cJSON_GetObjectItemCaseSensitive(mqtt_item, "username");
    if (mqtt_username_item) {
        if (!cJSON_IsString(mqtt_username_item) || !mqtt_username_item->valuestring) {
            ESP_LOGE(TAG, "Cannot get mqtt.username property");
            return ESP_ERR_INVALID_ARG;
        }

        _mqtt_username = mqtt_username_item->valuestring;

        ESP_LOGI(TAG, "MQTT username: %s", _mqtt_username.c_str());
    }

    auto mqtt_password_item = cJSON_GetObjectItemCaseSensitive(mqtt_item, "password");
    if (mqtt_password_item) {
        if (!cJSON_IsString(mqtt_password_item) || !mqtt_password_item->valuestring) {
            ESP_LOGE(TAG, "Cannot get mqtt.password property");
            return ESP_ERR_INVALID_ARG;
        }

        _mqtt_password = mqtt_password_item->valuestring;
    }

    auto rooms = cJSON_GetObjectItemCaseSensitive(*data, "rooms");
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
