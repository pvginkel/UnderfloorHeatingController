#include "includes.h"

#include "Arduino.h"
#include "Controller.h"
#include "DemoDriverProgram.h"
#include "HardMoveDriverProgram.h"
#include "TuningDriverProgram.h"

LOG_TAG(main);

#if ARDUINO_USB_CDC_ON_BOOT == 0
#error ARDUINO_USB_CDC_ON_BOOT must be enabled!
#endif

extern "C" void app_main(void) {
    delay(3000);

    ESP_LOGI(TAG, "Reset reason %d", esp_reset_reason());

    // The ESP32-S3 has three UARTs. By default, Serial0 refers to the
    // USB UART, not the first UART. Setting ARDUINO_USB_CDC_ON_BOOT fixes
    // this and the below is a runtime check for the same.
    ESP_ERROR_ASSERT((void*)&Serial != (void*)&Serial0);

    // The TMC2209 doesn't like pull up resistors on the UART lines. The ESP32
    // enables these by default.

    gpio_set_pull_mode((gpio_num_t)RX, GPIO_FLOATING);
    gpio_set_pull_mode((gpio_num_t)TX, GPIO_FLOATING);

    Controller controller(Serial0);

    controller.begin();

    const auto program = new TuningDriverProgram();

    // program->onFinished([&controller]() { controller.setProgram(new DemoDriverProgram()); });
    program->onFinished([&controller]() { controller.setProgram(new HardMoveDriverProgram()); });

    controller.setProgram(program);

    while (true) {
        controller.update();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
