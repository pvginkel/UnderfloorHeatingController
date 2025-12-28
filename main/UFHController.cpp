#include "support.h"

#include "UFHController.h"

#include "driver/gpio.h"

static const gpio_num_t PIN_MAPPING[] = {
    (gpio_num_t)CONFIG_DEVICE_ETA_6_PIN,  (gpio_num_t)CONFIG_DEVICE_ETA_5_PIN,  (gpio_num_t)CONFIG_DEVICE_ETA_4_PIN,
    (gpio_num_t)CONFIG_DEVICE_ETA_3_PIN,  (gpio_num_t)CONFIG_DEVICE_ETA_2_PIN,  (gpio_num_t)CONFIG_DEVICE_ETA_1_PIN,
    (gpio_num_t)CONFIG_DEVICE_ETA_7_PIN,  (gpio_num_t)CONFIG_DEVICE_ETA_8_PIN,  (gpio_num_t)CONFIG_DEVICE_ETA_9_PIN,
    (gpio_num_t)CONFIG_DEVICE_ETA_10_PIN, (gpio_num_t)CONFIG_DEVICE_ETA_11_PIN, (gpio_num_t)CONFIG_DEVICE_ETA_12_PIN,
};

LOG_TAG(UFHController);

esp_err_t UFHController::begin() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        ((1ull << CONFIG_DEVICE_MOTOR_RELAY_PIN) | (1ull << CONFIG_DEVICE_ETA_1_PIN) |
         (1ull << CONFIG_DEVICE_ETA_2_PIN) | (1ull << CONFIG_DEVICE_ETA_3_PIN) | (1ull << CONFIG_DEVICE_ETA_4_PIN) |
         (1ull << CONFIG_DEVICE_ETA_5_PIN) | (1ull << CONFIG_DEVICE_ETA_6_PIN) | (1ull << CONFIG_DEVICE_ETA_7_PIN) |
         (1ull << CONFIG_DEVICE_ETA_8_PIN) | (1ull << CONFIG_DEVICE_ETA_9_PIN) | (1ull << CONFIG_DEVICE_ETA_10_PIN) |
         (1ull << CONFIG_DEVICE_ETA_11_PIN) | (1ull << CONFIG_DEVICE_ETA_11_PIN));
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "gpio_config failed");

    ESP_RETURN_ON_ERROR(gpio_set_level((gpio_num_t)CONFIG_DEVICE_MOTOR_RELAY_PIN, 0), TAG, "gpio_set_level failed");

    for (int i = 0; i < ARRAY_SIZE(PIN_MAPPING); i++) {
        ESP_RETURN_ON_ERROR(gpio_set_level(PIN_MAPPING[i], 0), TAG, "gpio_set_level failed");
    }

    return ESP_OK;
}

void UFHController::set_motor_on(bool enabled) {
    ESP_LOGI(TAG, "Setting motor pin to %d", enabled ? 1 : 0);

    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)CONFIG_DEVICE_MOTOR_RELAY_PIN, enabled ? 1 : 0));
}

void UFHController::set_room_on(int index, bool enabled) {
    ESP_ERROR_ASSERT(_configuration);

    ESP_ERROR_ASSERT(index >= 0 && index < _configuration->get_rooms().size());

    const auto& room = _configuration->get_rooms()[index];

    // Room pin numbers are 1-12 and in the order of PIN_MAPPING.

    for (auto pin : room.get_pins()) {
        ESP_ERROR_ASSERT(pin >= 1 && pin <= ARRAY_SIZE(PIN_MAPPING));

        auto mapped_pin = PIN_MAPPING[pin - 1];

        ESP_LOGI(TAG, "Setting ETA pin %d to %d", (int)mapped_pin, enabled ? 1 : 0);

        ESP_ERROR_CHECK(gpio_set_level(mapped_pin, enabled ? 1 : 0));
    }
}
