/*
 * Task to process OBD PID values into an ABRP telemetry string
 */

#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#define LOG_LOCAL_NAME "telem t"
#include "log.h"

#include "freematics.h"

#include "abrp/telemetry.h"

// #include <ArduinoJson.h>

#include "tasks/common.h"
#include "tasks/telem.h"
#include "configs.h"



namespace tasks {
    namespace telem {

        const char TelemTask::name[] = "telem";
        const uint32_t TelemTask::memory = 6144;
        const uint8_t TelemTask::priority = 15;

        TelemTask::TelemTask(void * p) :
            Task(p),
            state(STATE_INIT),
            obdQueue(taskHandles.queueObd2Telem),
            sendQueue(taskHandles.queueTelem2Send),
            gpsQueue(taskHandles.queueGps2Telem),
            //telem
            //values
            //gps
            lastTimeStamp(0),
            currentTimeStamp(0),
            nextTimeStamp(0) {}

        void TelemTask::run() {
            while (true) {
                switch (state) {
                case STATE_INIT:
                    if (doInit()) {
                        LOGI("Telem task initialized");
                        state = STATE_WAITING_FOR_DATA;
                        break;
                    }
                    LOGE("Telem task failed to init");
                    delay(1000);
                    break;
                case STATE_WAITING_FOR_DATA:
                    if (waitForObdInQueue())
                        state = STATE_HAVE_DATA_TO_PROCESS;
                    break;
                case STATE_HAVE_DATA_TO_PROCESS:
                    if (getObdSetFromQueue() && makeTelemJson()) {
                        state = STATE_HAVE_JSON_TO_QUEUE;
                        break;
                    }
                    LOGE("Failed to convert telem to JSON");
                    break;
                case STATE_HAVE_JSON_TO_QUEUE:
                    if (putTelemInQueue()) {
                        if (currentTimeStamp < nextTimeStamp)
                            state = STATE_HAVE_DATA_TO_PROCESS;
                        else
                            state = STATE_WAITING_FOR_DATA;
                        break;
                    }
                    LOGE("Failed to put JSON in queue");
                    if (currentTimeStamp < nextTimeStamp)
                        state = STATE_HAVE_DATA_TO_PROCESS;
                    else
                        state = STATE_WAITING_FOR_DATA;
                    break;
                default:
                    LOGE("Invalid state!!!");
                    delay(1000);
                }
            }
        }

        bool TelemTask::doInit() {
            LOGI("Intializing telem task");
            telem.car_model = configs::PID_LIST_NAME;
            delay(500);
            return true;
        }

        bool TelemTask::waitForObdInQueue() {
            nextTimeStamp = ulTaskNotifyTake(pdTRUE, configs::RATE_OBD_READ);
            LOGI("Items available to read (Up to #%ld)", nextTimeStamp);
            return nextTimeStamp > 0;
        }

        bool TelemTask::getObdSetFromQueue() {
            currentTimeStamp = 0;
            for (uint8_t i = 0; i < configs::PID_LIST_LENGTH; i ++) {
                while (!xQueuePeek(obdQueue, &values[i], 10))
                    LOGE("Failed to get item from queue (%u items in queue)", uxQueueMessagesWaiting(taskHandles.queueObd2Telem));
                if (currentTimeStamp && values[i].sequence > currentTimeStamp) {
                    LOGW("Only received part of data set");
                    return false;
                } else if (!currentTimeStamp) {
                    currentTimeStamp = values[i].sequence;
                }
                //TODO: Figure out if there's a better way than writing to val twice...
                xQueueReceive(taskHandles.queueObd2Telem, &values[i], 0);
                LOGD("%lu: %s = %Lf", values[i].time, values[i].pid->name, values[i].value);
            }
            return true;
        }

        bool TelemTask::getGpsFromQueue() {
            //TODO: right now, we only ever get the most recent GPS fix.  What if we have a backup of OBD?
            //They're all going to be tagged with the same location right now.  In general, that's probably
            //not a big deal, as this task is intended to prevent a backup of data in the OBD queue,
            //so should mostly pull the two contemporaneously.  The sender thread will pull the complete
            //JSON and handle buffering, so that's where data should back up if there's no internet conection
            return xQueuePeek(gpsQueue, &gps, 0);
        }

        bool TelemTask::makeTelemJson() {
            telem.utc = values[0].time;
            if (getGpsFromQueue()) {
                telem.lat = gps.lat;
                telem.lon = gps.lng;
            } else {
                telem.lat.clear();
                telem.lon.clear();
            }
            for (uint8_t i = 0; i < configs::PID_LIST_LENGTH; i ++) {
                if (values[i].time > telem.utc() || values[i].time < telem.utc()) {
                    LOGW("Timestamp mismatch in telemetry set");
                    return false;
                }
                if (strcmp(values[i].pid->name, "soc") == 0)
                    telem.soc = values[i].value;
                else if (strcmp(values[i].pid->name, "speed") == 0)
                    telem.speed = values[i].value;
                else if (strcmp(values[i].pid->name, "lat") == 0)
                    telem.lat = values[i].value;
                else if (strcmp(values[i].pid->name, "lon") == 0)
                    telem.lon = values[i].value;
                else if (strcmp(values[i].pid->name, "is_charging") == 0)
                    telem.is_charging = values[i].value;
                else if (strcmp(values[i].pid->name, "power") == 0)
                    telem.power = values[i].value;
                else if (strcmp(values[i].pid->name, "is_dcfc") == 0)
                    telem.is_dcfc = values[i].value;
                else if (strcmp(values[i].pid->name, "battery_capacity") == 0)
                    telem.battery_capacity = values[i].value;
                else if (strcmp(values[i].pid->name, "soh") == 0)
                    telem.soh = values[i].value;
                else if (strcmp(values[i].pid->name, "elevation") == 0)
                    telem.elevation = values[i].value;
                else if (strcmp(values[i].pid->name, "ext_temp") == 0)
                    telem.ext_temp = values[i].value;
                else if (strcmp(values[i].pid->name, "batt_temp") == 0)
                    telem.batt_temp = values[i].value;
                else if (strcmp(values[i].pid->name, "voltage") == 0)
                    telem.voltage = values[i].value;
                else if (strcmp(values[i].pid->name, "current") == 0)
                    telem.current = values[i].value;
                else
                    LOGW("Unknown telemetry value: %s", values[i].pid->name);
            }
            LOGI("JSON (#%ld): %s", currentTimeStamp, telem.toJSON().c_str());
            return true;
        }

        bool TelemTask::putTelemInQueue() {
            if (!xQueueSendToBack(sendQueue, telem.toJSON().c_str(), 0)) {
                LOGE("Queue full");
                return false;
            }
            return true;
        }

    }
}
