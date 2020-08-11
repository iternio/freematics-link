#include "freematics.h"
// #ifdef BOARD_HAS_PSRAM
// #ifdef CONFIG_USING_ESPIDF
// #include <esp32/himem.h>
// #else
// #include <esp_himem.h>
// #endif
// #endif
// #include <lwip/apps/sntp.h>

#include "abrp.h"
#include "configs.h"
#include "system.h"
#include "util.h"
#include "tasks.h"
//TODO: Includes definitely need to be properly organized

using namespace util;

//TODO: Make sure to properly clean up namespaces (including :: prefix to indicate global ns)
//Shared Resources (TODO: Should any of these be on a specific thread's stack instead of the global heap?)
::FreematicsESP32 freematics;

void app() {
    Serial.begin(115200);
    // beep(880, 50);
    taskHandles.taskMain = xTaskGetCurrentTaskHandle();
    util::ptl();

    taskHandles.flags = xEventGroupCreate();

    util::ptl();
    xTaskCreate(tasks::init::task, "init", 8192, &freematics, 16, &taskHandles.taskInit);
    util::ptl();
    delay(200);
    util::ptl();
    delay(200);
    util::ptl();

    while (!ulTaskNotifyTake(pdFALSE, 100));

    util::ptl();
    taskHandles.taskNet = tasks::create<tasks::net::NetworkTask>();
    util::ptl();
    delay(200);
    util::ptl();
    delay(200);
    util::ptl();

    while (!ulTaskNotifyTake(pdFALSE, 100));

    log_v("System boot complete");
    util::ptl();
    Serial.println();

    taskHandles.queueObd2Telem = xQueueCreate(configs::PID_LIST_LENGTH * 10, sizeof(sys::obd::PIDValue));
    taskHandles.queueTelem2Send = xQueueCreate(10, sizeof(char) * 256);
    util::ptl();
    xTaskCreate(tasks::obd::task, "obd", 8192, freematics.link, 14, &taskHandles.taskObd);
    util::ptl();
    delay(200);
    util::ptl();
    delay(200);
    util::ptl();
    delay(600);
    util::ptl();
    xTaskCreate(tasks::telem::task, "telem", 8192, freematics.link, 13, &taskHandles.taskTelem);
    util::ptl();
    delay(200);
    util::ptl();
    delay(200);
    util::ptl();
    delay(600);
    util::ptl();
    taskHandles.taskSend = tasks::create<tasks::send::SenderTask>();
    util::ptl();
    delay(200);
    util::ptl();
    delay(200);
    util::ptl();
    delay(10000);
    util::ptl(true);

    while(true) {
        util::ptl();
        delay(30000);
    }
}

#if CONFIG_AUTOSTART_ARDUINO
void setup() { app(); }
void loop()  { delay(60000); }
#else
extern "C" void app_main() { app(); }
#endif
