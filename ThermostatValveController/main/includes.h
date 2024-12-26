#pragma once

#define _USE_MATH_DEFINES

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

using namespace std;

#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include <cctype>
#include <cstdarg>

// clang-format off
// See https://github.com/espressif/arduino-esp32/issues/4405.
#include <WiFiUdp.h>
// clang-format on

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_app_format.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "secrets.h"

// --

#include "Callback.h"
#include "support.h"

#define CONFIG_PIN_ENABLE 14
#define CONFIG_PIN_STEP 15
#define CONFIG_PIN_DIR 16
#define CONFIG_PIN_DIAG 17
#define CONFIG_DRIVER_R_SENSE 0.11f
#define CONFIG_DRIVER_ADDRESS 0
#define CONFIG_DRIVER_CURRENT 600
#define CONFIG_DRIVER_SG_THRESHOLD 190
#define CONFIG_DRIVER_TCOOL_THRESHOLD 150
#define CONFIG_DRIVER_MICROSTEPS 16
