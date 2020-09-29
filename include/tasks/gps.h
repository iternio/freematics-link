/*
 * Classes & functions to wrap reading GPS data
 */

#pragma once

#include <freertos/event_groups.h>
#include <freertos/queue.h>

#include "freematics.h"

#include "tasks/common.h"

namespace tasks {
    namespace gps {

        enum GpsTaskState {
            STATE_INIT = 0,
            STATE_NO_FIX,
            STATE_HAVE_FIX
        };

        class GpsTask : public Task {
        public:
            GpsTask(void * p);
            void run();

        private:
            bool doInit();
            bool checkForFix();
            bool getFix();
            bool getLocation();
            bool saveLocationToQueue();

            GpsTaskState state;
            QueueHandle_t & queue;
            EventGroupHandle_t & flags;
            uint8_t failures;
            ::Freematics * system;
            GPS_DATA * data;


        public:
            // static const TickType_t rate;   //TODO: Don't think we actually need this here, it's event driven (data in queue) rather than time driven

            static const char name[];
            static const uint32_t memory;
            static const uint8_t priority;
        };

    }
}
