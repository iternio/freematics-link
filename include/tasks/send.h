/*
 * Classes & functions to wrap sending telemetry to ABRP
 */

#pragma once

#include <Client.h>
#include <freertos/queue.h>

#include "tasks/common.h"
#include "system/http.h"

namespace tasks {
    namespace send {

        enum SenderTaskState {
            STATE_INIT = 0,
            STATE_NO_NETWORK,
            STATE_WAITING_FOR_DATA,
            // STATE_HAS_BUFFER_DATA_TO_SEND,
            // STATE_HAS_QUEUE_DATA_TO_SEND,
            // STATE_HAS_QUEUE_DATA_TO_BUFFER,
            STATE_DATA_TO_BUFFER,
            STATE_DATA_TO_SEND
        };

        class SenderTask : public Task {
        public:
            SenderTask(void * p = NULL);
            void run();

        private:
            bool doInit();
            bool checkForNetworkConnection();
            bool getNetworkConnection();
            bool getTelemInBuffer();
            bool waitForTelemInQueue();
            bool sendTelemToServer();
            bool saveTelemToBuffer();

            SenderTaskState state;
            QueueHandle_t & queue;
            EventGroupHandle_t & flags;
            ::Client * network;
            uint8_t type;
            uint8_t failures;
            sys::clt::HTTP http;
            //TODO: For now, we're good with just HTTP, but really want HTTPS, maybe want TCP or UDP
            //options for some connection types (such as BT)
            char data[256];

        public:
            // static const TickType_t rate;   //TODO: Don't think we actually need this here, it's event driven (data in queue) rather than time driven

            static const char name[];
            static const uint32_t memory;
            static const uint8_t priority;
        };

    }
}
