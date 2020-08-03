/*
 * Task to process OBD PID values into an ABRP telemetry string
 */

#include "freematics.h"

#include "abrp/telemetry.h"

// #include <ArduinoJson.h>

#include "tasks/common.h"
#include "tasks/telem.h"
#include "configs.h"

namespace tasks {
    namespace telem {

        void task(void * param) {
            log_d("Beginning Telemetry Task");
            #if ABRP_VERBOSE
            delay(200);
            #endif

            // TickType_t last = 0, current = 0;
            time_t last = 0, current = 0;
            // const TickType_t step = pdMS_TO_TICKS(configs::RATE_TELEM_CONVERT);
            log_d("Entering Telemetry Loop");
            while(true) {   //Main Loop
                while (!(last = ulTaskNotifyTake(pdTRUE, configs::RATE_OBD_READ)));
                log_d("Items available to read (Up to #%u)", last);
                do {  //Loop through backlog of queued telemetry sets
                    abrp::telemetry::Telemetry telem;
                    current = 0;
                    telem.car_model = configs::PID_LIST_NAME;
                    for (uint8_t i = 0; i < configs::PID_LIST_LENGTH; i ++) {  //Loop through queue entries for next telemetry set
                        sys::obd::PIDValue val;
                        while (!xQueuePeek(taskHandles.queueObd2Telem, &val, 10))
                            log_d("Failed to get item from queue (%u items in queue)", uxQueueMessagesWaiting(taskHandles.queueObd2Telem));
                        if (current && val.sequence > current)  //TODO: How to handle overflow...?
                            break;
                        else if (!current) {
                            current = val.sequence;
                            telem.utc = val.time;
                        }
                        //TODO: Figure out if there's a better way than writing to val twice...
                        xQueueReceive(taskHandles.queueObd2Telem, &val, 0);
                        // log_d("%u: %s = %Lf", val.time, val.pid->name, val.value);
                        if (strcmp(val.pid->name, "soc") == 0)
                            telem.soc = val.value;
                        else if (strcmp(val.pid->name, "speed") == 0)
                            telem.speed = val.value;
                        else if (strcmp(val.pid->name, "lat") == 0)
                            telem.lat = val.value;
                        else if (strcmp(val.pid->name, "lon") == 0)
                            telem.lon = val.value;
                        else if (strcmp(val.pid->name, "is_charging") == 0)
                            telem.is_charging = val.value;
                        else if (strcmp(val.pid->name, "power") == 0)
                            telem.power = val.value;
                        else if (strcmp(val.pid->name, "is_dcfc") == 0)
                            telem.is_dcfc = val.value;
                        else if (strcmp(val.pid->name, "battery_capacity") == 0)
                            telem.battery_capacity = val.value;
                        else if (strcmp(val.pid->name, "soh") == 0)
                            telem.soh = val.value;
                        else if (strcmp(val.pid->name, "elevation") == 0)
                            telem.elevation = val.value;
                        else if (strcmp(val.pid->name, "ext_temp") == 0)
                            telem.ext_temp = val.value;
                        else if (strcmp(val.pid->name, "batt_temp") == 0)
                            telem.batt_temp = val.value;
                        else if (strcmp(val.pid->name, "voltage") == 0)
                            telem.voltage = val.value;
                        else if (strcmp(val.pid->name, "current") == 0)
                            telem.current = val.value;
                        // else
                        //     log_e("Unknown telemetry value: %s", val.pid->name);
                    }
                    log_d("JSON (#%u): %s", current, telem.toJSON().c_str());
                    //TODO: Actuall enqueue the JSON for the next task to consume
                } while (current < last);
                // vTaskDelayUntil(&last, step);  This task delays in the wait for notify function, so don't need this
            }
        }
    }
}
