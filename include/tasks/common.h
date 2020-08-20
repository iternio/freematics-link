/*
 * Handles to task resources
 */

#pragma once

// #define LOG_LOCAL_NAME "tasks"
// #define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_DEBUG
#include "log.h"

#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <type_traits>

//TODO: Should these be wrapped in class objects?

namespace tasks {

    struct Handles {

        //Task Handles
        TaskHandle_t taskMain;
        TaskHandle_t taskObd;
        TaskHandle_t taskTelem;
        TaskHandle_t taskNet;
        TaskHandle_t taskSend;
        TaskHandle_t taskInit;
        TaskHandle_t taskGps;

        //Queue Handles
        QueueHandle_t queueObd2Telem;
        QueueHandle_t queueTelem2Send;
        QueueHandle_t queueGps2Telem;

        //Mutex Handles
        SemaphoreHandle_t mutexLink;
        SemaphoreHandle_t mutexClient;

        //Event Groups
        EventGroupHandle_t flags;
    };

    enum Flags {
        FLAG_NONE               = 0,
        FLAG_HAS_NETWORK        = bit(0),
        FLAG_NETWORK_IS_WIFI    = bit(1),
        FLAG_NETWORK_IS_SIM     = bit(2),
        FLAG_NETWORK_IS_BT      = bit(3)
    };

    //TODO: Fill out this class
    class Task {
    public:
        Task(void * p = NULL) : param(p) {};
        virtual ~Task() { vTaskDelete(NULL); } //TODO: not totally certain if this is good, bad, or indifferent
        virtual void run() = 0;

    private:
        void * param;

    public:
        static const char name[];
        static const uint32_t memory;
        static const uint8_t priority;
        static const bool deleteTaskWhenRunReturns = false;
    };

    // const bool Task::deleteTaskWhenRunReturns = false;

    template <class T> void run(void * param = NULL) {
        LOGI("Starting Task: %s", T::name);
        T t(param);
        while (!T::deleteTaskWhenRunReturns)
            t.run();
        vTaskDelete(NULL);
    }

    // template <class T> /* std::enable_if_t<std::is_base_of_v<Task, T>, */ TaskHandle_t/* > */ create(void * param = NULL);

    template <class T> TaskHandle_t create(void * param = NULL) {
        LOGD("Creating Task: %s (%u B, Pri %u)", T::name, T::memory, T::priority);
        TaskHandle_t h;
        xTaskCreate(run<T>, T::name, T::memory, param, T::priority, &h);
        return h;
    }

}

inline tasks::Handles taskHandles;

// #define LOG_TASK_REDIRECT(level, ...) ESP_LOG ## level (__VA_ARGS__)
// #define LOG_TASK_CONVERT(level, name, ...) LOG_TASK_REDIRECT(level, name, __VA_ARGS__)
// #define log_task(...) LOG_TASK_CONVERT(TASK_LOG_LEVEL, TASK_LOG_NAME, __VA_ARGS__)
