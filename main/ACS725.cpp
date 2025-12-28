#include "support.h"

#include "ACS725.h"

#include <cmath>
#include <cstdlib>

#include "NVSProperty.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

LOG_TAG(ACS725);

static NVSPropertyF32 nvs_zero_mv("zero_mv");
static NVSPropertyF32 nvs_noise_floor_a("noise_a");

// -------------------- USER-TUNABLE COMPILE-TIME CONFIG --------------------

// ADC attenuation / bitwidth
#define ACS725_ADC_ATTEN ADC_ATTEN_DB_12
#define ACS725_ADC_BITWIDTH SOC_ADC_DIGI_MAX_BITWIDTH

// Sensitivity (mV/A): depends on the exact ACS725 variant.
// The one in use is ACS725LLCTR-10AB-T which has 132 mV/A.
#define ACS725_SENSITIVITY_MV_PER_A 132.0f

#define ACS725_CLIP_LOW_MV 50
#define ACS725_CLIP_HIGH_MV 3250

// Continuous mode configuration
#define ACS725_SAMPLE_FREQ_HZ 1000
#define ACS725_DMA_BUFFER_SIZE 256
#define ACS725_DMA_BUFFER_COUNT 4

// -------------------------------------------------------------------------

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t* out_handle) {
    *out_handle = nullptr;
    esp_err_t ret;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = (adc_bitwidth_t)ACS725_ADC_BITWIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cfg, out_handle);
    if (ret == ESP_OK) {
        return true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cfg = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ACS725_ADC_BITWIDTH,
    };
    ret = adc_cali_create_scheme_line_fitting(&cfg, out_handle);
    if (ret == ESP_OK) {
        return true;
    }
#endif

    return false;
}

esp_err_t ACS725::begin(uint32_t report_interval_ms) {
    if (report_interval_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_unit_t unit;
    ESP_RETURN_ON_ERROR(adc_continuous_io_to_channel(CONFIG_DEVICE_CURRENT_MONITOR_PIN, &unit, &_channel), TAG,
                        "Failed to get ADC channel for GPIO %d", CONFIG_DEVICE_CURRENT_MONITOR_PIN);

    // Since we're sampling at 1 kHZ, a report interval in milliseconds
    // equates samples.
    _report_interval_ms = report_interval_ms;
    _window_samples = report_interval_ms;
    _ring_size = report_interval_ms;

    _ring = (float*)calloc(_ring_size, sizeof(float));
    if (!_ring) {
        return ESP_ERR_NO_MEM;
    }

    // ADC continuous mode init
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = ACS725_DMA_BUFFER_SIZE * ACS725_DMA_BUFFER_COUNT,
        .conv_frame_size = ACS725_DMA_BUFFER_SIZE,
    };
    ESP_RETURN_ON_ERROR(adc_continuous_new_handle(&adc_config, &_adc_handle), TAG, "adc_continuous_new_handle failed");

    // Configure the ADC pattern (single channel)
    adc_digi_pattern_config_t pattern = {
        .atten = ACS725_ADC_ATTEN,
        .channel = _channel,
        .unit = unit,
        .bit_width = ACS725_ADC_BITWIDTH,
    };

    adc_continuous_config_t dig_cfg = {
        .pattern_num = 1,
        .adc_pattern = &pattern,
        .sample_freq_hz = ACS725_SAMPLE_FREQ_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    ESP_RETURN_ON_ERROR(adc_continuous_config(_adc_handle, &dig_cfg), TAG, "adc_continuous_config failed");

    // ADC calibration init (required for mV conversion)
    if (!adc_calibration_init(unit, ACS725_ADC_ATTEN, &_cali_handle)) {
        ESP_LOGE(TAG, "ADC calibration not available; refusing to run (enable a supported cali scheme).");
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Start continuous conversion
    ESP_RETURN_ON_ERROR(adc_continuous_start(_adc_handle), TAG, "adc_continuous_start failed");

    xTaskCreate([](void* self) { ((ACS725*)self)->task_loop(); }, "acs725", CONFIG_MAIN_TASK_STACK_SIZE, this, 2,
                nullptr);

    ESP_LOGI(TAG, "Started: unit=%d channel=%d window=%u samples @ %dHz, SENS=%.1fmV/A", (int)unit, (int)_channel,
             (unsigned)_ring_size, ACS725_SAMPLE_FREQ_HZ, (double)ACS725_SENSITIVITY_MV_PER_A);

    return ESP_OK;
}

void ACS725::task_loop() {
    load_state();

    if (_zero_mv == 0 || true) {
        ESP_LOGI(TAG, "Starting calibration");
        _calibration = new ACS725Calibration({});
        _calibration->reset();
    } else {
        ESP_LOGI(TAG, "Loaded calibration: zero_mv=%.1f noise_floor=%.3fA", _zero_mv, _noise_floor_a);
    }

    uint8_t buffer[ACS725_DMA_BUFFER_SIZE];
    uint32_t bytes_read = 0;

    auto next_report_ms = esp_get_millis() + _report_interval_ms;

    while (true) {
        const auto now_ms = esp_get_millis();
        const auto delay = next_report_ms - now_ms;

        if (delay <= 0) {
            // Don't report values while calibration is in progress.
            if (!_calibration) {
                // Report the current RMS, corrected for noise floor.
                const float rms_raw = std::sqrt(_ring_sum / (float)_ring_size);
                const float rms = std::sqrt(std::max(0.0f, rms_raw * rms_raw - _noise_floor_a * _noise_floor_a));

                ESP_LOGI(TAG, "Reported %f A (raw %f, noise floor %f)", rms, rms_raw, _noise_floor_a);

                _current_changed.queue(_queue, rms);
            }

            // Shift the next report window.
            do {
                next_report_ms += _report_interval_ms;
            } while (next_report_ms < now_ms);
        } else {
            esp_err_t err = adc_continuous_read(_adc_handle, buffer, sizeof(buffer), &bytes_read, delay);

            if (err == ESP_OK && bytes_read > 0) {
                process_samples(buffer, bytes_read);
            } else if (err != ESP_ERR_TIMEOUT) {
                ESP_LOGW(TAG, "adc_continuous_read failed: %s", esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
}

void ACS725::load_state() {
    _zero_mv = 0;
    _noise_floor_a = 0;

    nvs_handle_t handle;
    auto err = nvs_open("acs725", NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return;
    }
    ESP_ERROR_CHECK(err);

    _zero_mv = nvs_zero_mv.get(handle, 0);
    _noise_floor_a = nvs_noise_floor_a.get(handle, 0);

    nvs_close(handle);
}

void ACS725::save_state() {
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("acs725", NVS_READWRITE, &handle));

    nvs_zero_mv.set(handle, _zero_mv);
    nvs_noise_floor_a.set(handle, _noise_floor_a);

    nvs_close(handle);
}

void ACS725::process_samples(const uint8_t* buffer, uint32_t len) {
    auto* data = (adc_digi_output_data_t*)buffer;
    size_t count = len / sizeof(adc_digi_output_data_t);

    for (size_t i = 0; i < count; i++) {
        int raw = data[i].type2.data;
        int mv = 0;

        if (adc_cali_raw_to_voltage(_cali_handle, raw, &mv) != ESP_OK) {
            continue;
        }

        if (_calibration) {
            // Calibration is still in progress.
            const auto done = _calibration->add_sample(mv);
            if (done) {
                const auto result = _calibration->result();

                _zero_mv = result.zero_mv;
                _noise_floor_a = result.final_std_dev / ACS725_SENSITIVITY_MV_PER_A;

                ESP_LOGI(TAG, "Calibration result: success=%s zero_mv=%.1f noise_floor=%.3fA",
                         result.success ? "YES" : "NO", _zero_mv, _noise_floor_a);

                ESP_ERROR_ASSERT(result.success);

                save_state();

                delete _calibration;
                _calibration = nullptr;
            }
        } else {
            // Convert to current (A), then square for RMS calculation
            const float current_a = ((float)mv - _zero_mv) / ACS725_SENSITIVITY_MV_PER_A;
            window_push(current_a * current_a);

            _sample_count++;
            if (_sample_count >= _window_samples) {
                _sample_count = 0;
            }
        }
    }
}

void ACS725::window_push(float current_squared) {
    _ring_sum -= _ring[_ring_idx];
    _ring[_ring_idx] = current_squared;
    _ring_sum += current_squared;

    if (++_ring_idx >= _ring_size) {
        _ring_idx = 0;

        // Recalculate sum from scratch to avoid floating-point accumulation error.
        float sum = 0.0f;
        for (size_t i = 0; i < _ring_size; i++) {
            sum += _ring[i];
        }
        _ring_sum = sum;
    }
}
