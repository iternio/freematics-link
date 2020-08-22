/*
 * Task to get GPS location data
 */

#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE
#define LOG_LOCAL_NAME "gps t"
#include "log.h"

#include "freematics.h"

#include "tasks/common.h"
#include "tasks/gps.h"

//TODO: The freematics library seems to drastically simplify things and lacks documentation
//The GPS functions on the FreematicsESP32 object are very high level, and have no way to check
//the fix status.  When the fix is lost, it will just report the stale lat/lon data, but the
//timestamp continues to update so long as at least a single satellite signal is received.
//The number of sats usually indicates a lack of fix, but this just reports the number of sats
//the atnenna is receiving from, and occaisionally appears to retun stale data still but 3 or 4
//sats.  FOr now, ignoring and assuming this is only occaisional, and will address it later.
//Also, the hdop value seems to just be an incrementing number, and it's not clear that it
//is actually a value indicating the precision of the fix.

//TODO: Delete stale fix from queue after a reasonable timeout

namespace tasks {
    namespace gps {

        const char GpsTask::name[] = "gps";
        const uint32_t GpsTask::memory = 8192;
        const uint8_t GpsTask::priority = 13;

        char buffer[128];

        // const TickType_t GpsTask::rate = pdMS_TO_TICKS(10000);

        GpsTask::GpsTask(void * p) :
            Task(p),
            state(STATE_INIT),
            queue(taskHandles.queueGps2Telem),
            flags(taskHandles.flags),
            failures(0),
            system((::Freematics *)p),
            data(NULL) {}

        void GpsTask::run() {
            while (true) {
                switch (state) {
                case STATE_INIT:
                    if (!doInit()) {
                        LOGW("GPS task failed to init");
                        delay(2500);
                        break;
                    }
                    LOGI("GPS initialized");
                    state = STATE_NO_FIX;
                    break;
                case STATE_NO_FIX:
                    if (!getFix()) {
                        LOGI("Could not find location fix");
                        delay(2500);
                        break;
                    }
                    state = STATE_HAVE_FIX;
                    failures = 0;
                    break;
                case STATE_HAVE_FIX:
                    if (!getLocation()) {
                        LOGD("Failed to get location");
                        if (failures ++ >= 30 || !checkForFix()) {
                            LOGW("No location fix");
                            state = STATE_NO_FIX;
                            break;
                        }
                    } else {
                        failures = 0;
                        if (!saveLocationToQueue())
                            LOGE("Failed to put location in queue");
                    }
                    delay(200);
                    break;
                default:
                    LOGW("Invalid state!!!");
                    delay(2500);
                }
            }
        }

        bool GpsTask::doInit() {
            LOGD("Initializing GPS antenna");
            return system->gpsBegin();
        }

        bool GpsTask::checkForFix() {
            //TODO: At the moment, there is no direct way to do this other than get data and see what it says
            return system->gpsGetData(&data) && data->sat >= 3;
        }

        bool GpsTask::getFix() {
            //TODO: The Freematics library doesn't have a clear way to do this, so just sit and wait until it finds it on its own
            while (!checkForFix())
                delay(5000);
            return true;
        }

        bool GpsTask::getLocation() {
            if (!system->gpsGetData(&data))
                return false;
            LOGV("GPS: ts=%u d=%u t=%u lat=%f lon=%f alt=%f spd=%f hdg=%u hdop=%u sat=%u snt=%u err=%u", data->ts, data->date, data->time, data->lat, data->lng, data->alt, data->speed, data->heading, data->hdop, data->sat, data->sentences, data->errors);
            return true;
        }

        bool GpsTask::saveLocationToQueue() {
            return xQueueOverwrite(queue, data);
        }

    }
}
