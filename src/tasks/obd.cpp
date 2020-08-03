/*
 * Task to access PIDs via OBD connection
 */

#include "freematics.h"

#include "tasks/obd.h"
#include "tasks/common.h"
#include "system/obd.h"
#include "configs.h"
#include "abrp/telemetry.h"
#include "util.h"
#include <charconv>

// extern tasks::Handles h;

namespace tasks {
    namespace obd {

        void task(void* param) {
            log_d("Beginning OBD Task");
            CLink* link = (CLink*)param;
            sys::obd::OBD obd;
            char buffer[128];
            char * data = buffer;
            const sys::obd::PID * pid = 0;
            uint8_t queued = 0;
            time_t utc = 0;
            uint32_t seq = 0;
            log_d("Initializing OBD connection");
            obd.begin(link);

            while (true) {
                TickType_t last = xTaskGetTickCount();
                const TickType_t step = pdMS_TO_TICKS(configs::RATE_OBD_READ);
                log_d("Entering OBD Loop");
                while (obd.state() == OBD_CONNECTED && obd.errors < 3) {
                    queued = 0;
                    seq ++;
                    time(&utc);
                    log_d("Reading %u PIDs for %s at %u (#%u)", configs::PID_LIST_LENGTH, configs::PID_LIST_NAME, utc, seq);
                    for (uint8_t idx = 0; idx < configs::PID_LIST_LENGTH;  idx++) {
                        pid = &configs::PID_LIST[idx];
                        // log_d("Reading PID %02X %0*X - %s", pid->mode, (pid->mode > 0x09 && pid->id > 0xFF ? 4 : 2), pid->id, pid->name);
                        obd.readPIDRaw(pid->mode, pid->id, buffer, sizeof(buffer));
                        // log_d("Read %s", buffer);
                        //TODO: Check for error before trying to parse!
                        data = obd.parseRaw(buffer, *pid);
                        if (!data)
                            continue;
                        sys::obd::PIDValue value = {pid, utc, seq, "", obd.normalizePIDFromFormula(pid->formula, data)};
                        // log_v("%s at %X", value.pid->name, &value.pid);
                        strcpy(value.raw, data);
                        // log_d("%s: Parsed value as %Lf", value.raw, value.value);
                        log_d("Read PID %02X %0*X - %s = %Lf (%s)", pid->mode, (pid->mode > 0x09 && pid->id > 0xFF ? 4 : 2), pid->id, pid->name, value.value, value.raw);
                        if (!xQueueSendToBack(taskHandles.queueObd2Telem, &value, 0))
                            log_d("Failed put value in queue");
                        else
                            queued++;
                    }
                    if (queued && taskHandles.taskTelem)
                        xTaskNotify(taskHandles.taskTelem, seq, eSetValueWithOverwrite);
                    vTaskDelayUntil(&last, step);
                    // Serial.println("...");
                }
                log_d("Attempting to connect to OBD");
                while (!obd.init()) {
                    log_d("Could not establish OBD connection, waiting...");
                    vTaskDelayUntil(&last, step * 20);
                }
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

/* BOILERPLATE:
namespace tasks {
    namespace obd {

        void task(void* param) {
            log_d("Beginning OBD Task");
            QueueHandle_t queue = param;
            abrp::telemetry::Telemetry telem;
            init();

            TickType_t last = xTaskGetTickCount();
            const TickType_t step = pdMS_TO_TICKS(configs::RATE_OBD_READ);
            log_d("Entering OBD Loop");
            while (true) {
                //Read OBD
                if (1) { //OBD is valid
                    xQueueOverwrite(queue, &telem);
                }
                vTaskDelayUntil(&last, step);
            }

        }

        bool init() {

        }

    }
}
*/
