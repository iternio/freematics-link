/*
 * Task to access PIDs via OBD connection
 */

#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#define LOG_LOCAL_NAME "obd t"
#include "log.h"

#include "freematics.h"

#include "tasks/obd.h"
#include "tasks/common.h"
#include "system/obd.h"
#include "configs.h"
#include "abrp/telemetry.h"
#include "util.h"
#include <charconv>



namespace tasks {
    namespace obd {

        void task(void* param) {
            LOGI("Beginning OBD Task");
#if ABRP_VERBOSE
            delay(200);
            #endif

            CLink* link = (CLink*)param;
            sys::obd::OBD obd;
            char buffer[128];
            char * data = buffer;
            const sys::obd::PID * pid = 0;
            uint8_t queued = 0;
            time_t utc = 0;
            uint32_t seq = 0;
            LOGI("Initializing OBD connection");
            obd.begin(link);

            while (true) {
                TickType_t last = xTaskGetTickCount();
                const TickType_t step = pdMS_TO_TICKS(configs::RATE_OBD_READ);
                LOGD("Entering OBD Loop");
                while (obd.state() == OBD_CONNECTED && obd.errors < 3) {
                    queued = 0;
                    seq ++; //TODO: How to handle overflow...?
                    time(&utc);
                    LOGI("Reading %u PIDs for %s at %ld (#%u, V=%f)", configs::PID_LIST_LENGTH, configs::PID_LIST_NAME, utc, seq, obd.getVoltage());
                    for (uint8_t idx = 0; idx < configs::PID_LIST_LENGTH;  idx++) {
                        pid = &configs::PID_LIST[idx];
                        LOGD("Reading PID %02X %0*X - %s", pid->mode, (pid->mode > 0x09 && pid->id > 0xFF ? 4 : 2), pid->id, pid->name);
                        obd.readPIDRaw(pid->mode, pid->id, buffer, sizeof(buffer));
                        LOGV("Read %s", buffer);
                        //TODO: Check for error before trying to parse!
                        data = obd.parseRaw(buffer, *pid);
                        if (!data)
                            continue;
                        sys::obd::PIDValue value = {pid, utc, seq, "", obd.normalizePIDFromFormula(pid->formula, data)};
                        LOGV("%s at %p", value.pid->name, &value.pid);
                        strcpy(value.raw, data);
                        LOGV("%s: Parsed value as %Lf", value.raw, value.value);
                        LOGD("Read PID %02X %0*X - %s = %Lf (%s)", pid->mode, (pid->mode > 0x09 && pid->id > 0xFF ? 4 : 2), pid->id, pid->name, value.value, value.raw);
                        if (!xQueueSendToBack(taskHandles.queueObd2Telem, &value, 0))
                            log_e( "Failed put value in queue", esp_log_timestamp(), LOG_LOCAL_NAME);
                        else
                            queued++;
                    }
                    if (queued && taskHandles.taskTelem)
                        xTaskNotify(taskHandles.taskTelem, seq, eSetValueWithOverwrite);
                    vTaskDelayUntil(&last, step);
                    // Serial.println("...");
                }
                LOGI("Attempting to connect to OBD");
                while (!obd.init()) {
                    LOGW("Could not establish OBD connection, waiting...");
                    vTaskDelayUntil(&last, step * 20);
                }
                obd.getVIN(buffer, sizeof(buffer));
                LOGI("VIN: %s Voltage: %g", buffer, obd.getVoltage());
                obd.link->sendCommand("ATH1\r", buffer, sizeof(buffer), OBD_TIMEOUT_SHORT);
                obd.link->sendCommand("AT@1\r", buffer, sizeof(buffer), OBD_TIMEOUT_SHORT);
                obd.link->sendCommand("AT@2\r", buffer, sizeof(buffer), OBD_TIMEOUT_SHORT);
                obd.link->sendCommand("ATCS\r", buffer, sizeof(buffer), OBD_TIMEOUT_SHORT);
                obd.link->sendCommand("ATDP\r", buffer, sizeof(buffer), OBD_TIMEOUT_SHORT);
            }

        }

        // bool init(CLink* link) {

        // }

    }
}
