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

extern tasks::Handles h;

namespace tasks {
    namespace obd {

        void task(void* param) {
            log_d("Beginning OBD Task");
            CLink* link = (CLink*)param;
            abrp::telemetry::Telemetry telem;
            sys::obd::OBD obd;
            util::FormulaParser parser;
            char buffer[128];
            char * data;
            uint16_t val;
            uint8_t idwidth;
            log_d("Initializing OBD connection");
            obd.begin(link);

            while (true) {
                TickType_t last = xTaskGetTickCount();
                const TickType_t step = pdMS_TO_TICKS(configs::RATE_OBD_READ);
                log_d("Entering OBD Loop");
                while (obd.state() == OBD_CONNECTED && obd.errors < 3) {
                    log_d("Reading %u PIDs for %s", configs::PID_LIST_LENGTH, configs::PID_LIST_NAME);
                    for (uint8_t idx = 0; idx < configs::PID_LIST_LENGTH;  idx++) {
                        const sys::obd::PID & pid = configs::PID_LIST[idx];
                        idwidth = (pid.mode > 0x09 && pid.id > 0xFF ? 4 : 2);
                        log_d("Reading PID %02X %0*X - %s", pid.mode, idwidth, pid.id, pid.name);
                        obd.readPIDRaw(pid.mode, pid.id, buffer, sizeof(buffer));
                        log_d("Read %s", buffer);
                        data = &buffer[0];
                        util::str_remove(data, " \r\n>");
                        //Check header
                        std::from_chars(data, data + 3, val, 16);
                        if (val != pid.header) {
                            log_d("%.3s: Received header (%03X) does not match expected header (%03X)", data, val, pid.header);
                            //TODO: Skip it if the header doesn't match?
                        }
                        //Get Length
                        data += 3;
                        std::from_chars(data, data + 2, val, 16);
                        if (val != strlen(data) / 2 - 1) {
                            log_d("%.2s: Received %u bytes, but expecting %u bytes of response (%u bytes of data)", data, strlen(data) / 2 - 1, val, val - 1 - idwidth / 2);
                            continue;
                        }
                        //Check mode
                        data += 2;
                        std::from_chars(data, data + 2, val, 16);
                        if (val != pid.mode + 0x40) {
                            log_d("%.2s: Received mode (%02X) does not match expected mode (%02X)", data, val, pid.mode + 0x40);
                            continue;
                        }
                        //Check PID
                        data += 2;
                        std::from_chars(data, data + idwidth, val, 16);
                        if (val != pid.id) {
                            log_d("%.*s: Received PID (%0*X) does not match expected mode (%0*X)", idwidth, data, idwidth, val, idwidth, pid.id);
                            continue;
                        }
                        //Parse data
                        data += idwidth;
                        sys::obd::PIDValue value = {pid, last, "", parser.parse(pid.formula, data)};
                        strcpy(value.raw, data);
                        log_d("%s: Parsed value as %Lf", value.raw, value.value);
                        // if (!xQueueSendToBack(taskHandles.queueObd2Telem, &value, 0))
                        //     log_d("Failed put value in queue");
                    }
                    Serial.println();
                    vTaskDelayUntil(&last, step);
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
