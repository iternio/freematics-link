/*
 * Classes & functions to wrap processing OBD data into ABRP telemetry
 */

#pragma once

#include "freertos/queue.h"

#include "abrp/telemetry.h"

#include "tasks/common.h"

#include "configs.h"

namespace tasks {
    namespace telem {

        enum TelemTaskState {
            STATE_INIT = 0,
            STATE_WAITING_FOR_DATA,
            STATE_HAVE_DATA_TO_PROCESS,
            STATE_HAVE_JSON_TO_QUEUE
        };

        class TelemTask : public Task {
        public:
            TelemTask(void * p = NULL);
            void run();

        private:
            bool doInit();
            bool waitForObdInQueue();
            bool getObdSetFromQueue();
            bool makeTelemJson();
            bool putTelemInQueue();

            TelemTaskState state;
            QueueHandle_t & obdQueue;
            QueueHandle_t & sendQueue;
            abrp::telemetry::Telemetry telem;
            sys::obd::PIDValue values[configs::PID_LIST_LENGTH];
            time_t lastTimeStamp;
            time_t currentTimeStamp;
            time_t nextTimeStamp;

        public:
            static const char name[];
            static const uint32_t memory;
            static const uint8_t priority;

        };

        void task(void* param);
        bool init();

    }
}
