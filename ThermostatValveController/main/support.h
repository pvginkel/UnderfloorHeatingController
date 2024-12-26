#pragma once

#ifdef LV_SIMULATOR
#include <ctime>
#endif

#include "cJSON.h"

#define esp_get_millis() uint32_t(esp_timer_get_time() / 1000ull)

string format(const char* fmt, ...);
int getisoweek(tm& time_info);

#ifdef NDEBUG
#define ESP_ERROR_ASSERT(x) \
    do {                    \
        (void)sizeof((x));  \
    } while (0)
#elif defined(CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT)
#define ESP_ERROR_ASSERT(x)   \
    do {                      \
        if (unlikely(!(x))) { \
            abort();          \
        }                     \
    } while (0)
#else
#define ESP_ERROR_ASSERT(x)                                                                                    \
    do {                                                                                                       \
        if (unlikely(!(x))) {                                                                                  \
            printf("ESP_ERROR_ASSERT failed");                                                                 \
            printf(" at %p\n", __builtin_return_address(0));                                                   \
            printf("file: \"%s\" line %d\nfunc: %s\nexpression: %s\n", __FILE__, __LINE__, __ASSERT_FUNC, #x); \
            abort();                                                                                           \
        }                                                                                                      \
    } while (0)
#endif

#define ESP_TIMER_MS(v) ((v) * 1000)
#define ESP_TIMER_SECONDS(v) ESP_TIMER_MS((v) * 1000)

#define ESP_ERROR_CHECK_JUMP(x, label)                                     \
    do {                                                                   \
        esp_err_t err_rc_ = (x);                                           \
        if (unlikely(err_rc_ != ESP_OK)) {                                 \
            ESP_LOGE(TAG, #x " failed with %s", esp_err_to_name(err_rc_)); \
            goto label;                                                    \
        }                                                                  \
    } while (0)

bool iequals(const string& a, const string& b);
int hextoi(char c);

#define LOG_TAG(v) static const char* TAG = #v

class cJSON_Data {
    cJSON* _data;

public:
    cJSON_Data(cJSON* data) : _data(data) {}
    cJSON_Data(const cJSON_Data& other) = delete;
    cJSON_Data(cJSON_Data&& other) noexcept = delete;
    cJSON_Data& operator=(const cJSON_Data& other) = delete;
    cJSON_Data& operator=(cJSON_Data&& other) noexcept = delete;

    ~cJSON_Data() {
        if (_data) {
            cJSON_Delete(_data);
        }
    }

    cJSON* operator*() const { return _data; }
};

#ifdef LV_SIMULATOR
#define IRAM_ATTR

#define localtime_r(timep, result) localtime_s(result, timep)
#endif

#ifndef LV_SIMULATOR

esp_err_t esp_http_download_string(const esp_http_client_config_t& config, string& target, size_t max_length = 0,
                                   const char* authorization = nullptr);
esp_err_t esp_http_upload_string(const esp_http_client_config_t& config, const char* const data);
char const* esp_reset_reason_to_name(esp_reset_reason_t reason);

#endif
