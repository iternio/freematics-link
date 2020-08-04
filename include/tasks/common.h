/*
 * Handles to task resources
 */

#pragma once

#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

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

        //Queue Handles
        QueueHandle_t queueObd2Telem;
        QueueHandle_t queueTelem2Send;

        //Mutex Handles
        SemaphoreHandle_t mutexLink;
        SemaphoreHandle_t mutexClient;

        //Event Groups
        EventGroupHandle_t flags;
    };

    enum Flags {
        FLAG_HAS_NETWORK        = bit(0),
        FLAG_NETWORK_IS_WIFI    = bit(1),
        FLAG_NETWORK_IS_SIM     = bit(2),
        FLAG_NETWORK_IS_BT      = bit(3)
    };

}

inline tasks::Handles taskHandles;
