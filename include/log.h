/*
 * Wrapper for ESP logging library with some tweaks
 */

#pragma once

#define _STR(x) #x
#define STR(x) _STR(x)

#ifndef LOG_LOCAL_NAME
#define LOG_LOCAL_NAME "main"
#endif

#ifndef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#endif

#pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL))

#define CONFIG_USE_ARDUHAL_LOGGING 1
#define CONFIG_USE_CUSTOM_ARDUHAL_FORMAT 1

#include <esp32-hal-log.h>

#ifdef CONFIG_USE_ARDUHAL_LOGGING

#if LOG_LOCAL_LEVEL > ARDUHAL_LOG_LEVEL
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " greater than default")

#if LOG_LOCAL_LEVEL >= ARDUHAL_LOG_LEVEL_VERBOSE
#undef log_v
#define log_v(format, ...) log_printf(ARDUHAL_LOG_FORMAT(V, format), ##__VA_ARGS__)
#endif

#if LOG_LOCAL_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
#undef log_d
#define log_d(format, ...) log_printf(ARDUHAL_LOG_FORMAT(D, format), ##__VA_ARGS__)
#endif

#if LOG_LOCAL_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
#undef log_i
#define log_i(format, ...) log_printf(ARDUHAL_LOG_FORMAT(I, format), ##__VA_ARGS__)
#endif

#if LOG_LOCAL_LEVEL >= ARDUHAL_LOG_LEVEL_WARNING
#undef log_w
#define log_w(format, ...) log_printf(ARDUHAL_LOG_FORMAT(W, format), ##__VA_ARGS__)
#endif

#if LOG_LOCAL_LEVEL >= ARDUHAL_LOG_LEVEL_ERROR
#undef log_e
#define log_e(format, ...) log_printf(ARDUHAL_LOG_FORMAT(E, format), ##__VA_ARGS__)
#endif

#if LOG_LOCAL_LEVEL >= ARDUHAL_LOG_LEVEL_NONE
#undef log_n
#define log_n(format, ...) log_printf(ARDUHAL_LOG_FORMAT(E, format), ##__VA_ARGS__)
#endif

#elif LOG_LOCAL_LEVEL < ARDUHAL_LOG_LEVEL
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " less than default")

#if LOG_LOCAL_LEVEL < ARDUHAL_LOG_LEVEL_VERBOSE
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " less than " STR(ARDUHAL_LOG_LEVEL_VERBOSE))
#undef log_v
#define log_v(format, ...) do {} while(0)
#endif

#if LOG_LOCAL_LEVEL < ARDUHAL_LOG_LEVEL_DEBUG
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " less than " STR(ARDUHAL_LOG_LEVEL_DEBUG))
#undef log_d
#define log_d(format, ...) do {} while(0)
#endif

#if LOG_LOCAL_LEVEL < ARDUHAL_LOG_LEVEL_INFO
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " less than " STR(ARDUHAL_LOG_LEVEL_INFO))
#undef log_i
#define log_i(format, ...) do {} while(0)
#endif

#if LOG_LOCAL_LEVEL < ARDUHAL_LOG_LEVEL_WARNING
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " less than " STR(ARDUHAL_LOG_LEVEL_WARNING))
#undef log_w
#define log_w(format, ...) do {} while(0)
#endif

#if LOG_LOCAL_LEVEL < ARDUHAL_LOG_LEVEL_ERROR
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " less than " STR(ARDUHAL_LOG_LEVEL_ERROR))
#undef log_e
#define log_e(format, ...) do {} while(0)
#endif

#if LOG_LOCAL_LEVEL < ARDUHAL_LOG_LEVEL_NONE
// #pragma message(LOG_LOCAL_NAME " local " STR(LOG_LOCAL_LEVEL) " less than " STR(ARDUHAL_LOG_LEVEL_NONE))
#undef log_n
#define log_n(format, ...) do {} while(0)
#endif

#endif

#ifdef CONFIG_USE_CUSTOM_ARDUHAL_FORMAT
#undef ARDUHAL_LOG_FORMAT
#define ARDUHAL_LOG_FORMAT(letter, format)  ARDUHAL_LOG_COLOR_ ## letter "[" #letter "](%d)  %s: " format ARDUHAL_LOG_RESET_COLOR "\r\n"

#define LOGE( format, ... ) log_e( format, esp_log_timestamp(), LOG_LOCAL_NAME, ##__VA_ARGS__)
#define LOGW( format, ... ) log_w( format, esp_log_timestamp(), LOG_LOCAL_NAME, ##__VA_ARGS__)
#define LOGI( format, ... ) log_i( format, esp_log_timestamp(), LOG_LOCAL_NAME, ##__VA_ARGS__)
#define LOGD( format, ... ) log_d( format, esp_log_timestamp(), LOG_LOCAL_NAME, ##__VA_ARGS__)
#define LOGV( format, ... ) log_v( format, esp_log_timestamp(), LOG_LOCAL_NAME, ##__VA_ARGS__)

#else

#define LOGE( format, ... ) log_e(format, ##__VA_ARGS__)
#define LOGW( format, ... ) log_w(format, ##__VA_ARGS__)
#define LOGI( format, ... ) log_i(format, ##__VA_ARGS__)
#define LOGD( format, ... ) log_d(format, ##__VA_ARGS__)
#define LOGV( format, ... ) log_v(format, ##__VA_ARGS__)

#endif

#else

#define LOGE( format, ... ) ESP_LOGE( LOG_LOCAL_NAME, format, ##__VA_ARGS__)
#define LOGW( format, ... ) ESP_LOGW( LOG_LOCAL_NAME, format, ##__VA_ARGS__)
#define LOGI( format, ... ) ESP_LOGI( LOG_LOCAL_NAME, format, ##__VA_ARGS__)
#define LOGD( format, ... ) ESP_LOGD( LOG_LOCAL_NAME, format, ##__VA_ARGS__)
#define LOGV( format, ... ) ESP_LOGV( LOG_LOCAL_NAME, format, ##__VA_ARGS__)

#endif
