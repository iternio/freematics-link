/*
 * Handles to task resources
 */

#pragma once

#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

//TODO: Should these be wrapped in class objects?

namespace tasks {

    //Task Handles
    TaskHandle_t taskMain;
    TaskHandle_t taskObd;
    TaskHandle_t taskTelem;
    TaskHandle_t taskNet;
    TaskHandle_t taskSend;

    //Queue Handles
    QueueHandle_t queueObd2Telem;
    QueueHandle_t queueTelem2Send;

    //Mutex Handles
    SemaphoreHandle_t mutexLink;
    SemaphoreHandle_t mutexClient;

}
