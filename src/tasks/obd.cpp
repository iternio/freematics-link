/*
 * Task to access PIDs via OBD connection
 */

#include "tasks/obd.h"
#include "tasks/common.h"
#include "system/obd.h"
#include "configs.h"
#include "abrp/telemetry.h"

#include <FreematicsPlus.h>

namespace tasks {
    namespace obd {

        void task(void* param) {
            log_d("Beginning OBD Task");
            CLink* link = (CLink*)param;
            abrp::telemetry::Telemetry telem;
            sys::obd::OBD obd;
            log_d("Initializing OBD connection");
            obd.begin(link);

            while (true) {
                TickType_t last = xTaskGetTickCount();
                const TickType_t step = pdMS_TO_TICKS(configs::RATE_OBD_READ);
                log_d("Entering OBD Loop");
                while (obd.state() == OBD_CONNECTED && obd.errors < 3) {
                    //Read OBD
                    if (1) { //OBD is valid
                        xQueueOverwrite(queueObd2Telem, &telem);
                    }
                    vTaskDelayUntil(&last, step);
                    //TODO: check OBD connection & reinitialize as necessary
                }
                log_d("Attempting to connect to OBD");
                while (!obd.init()) {
                    log_d("Could not establish OBD connection, waiting...");
                    vTaskDelayUntil(&last, step * 20);
                }
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
