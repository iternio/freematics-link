#define LOG_LOCAL_NAME "main"
#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#include "log.h"

#include "freematics.h"

#include "abrp.h"
#include "configs.h"
#include "system.h"
#include "util.h"
#include "tasks.h"
//TODO: Includes definitely need to be properly organized

// using namespace util;

//TODO: Make sure to properly clean up namespaces (including :: prefix to indicate global ns)
//Shared Resources (TODO: Should any of these be on a specific thread's stack instead of the global heap?)
// ::FreematicsESP32 freematics;
::Freematics freematics;

void app() {
    LOGI("Application started");
    Serial.begin(115200);
    // util::beep(880, 50);

    taskHandles.taskMain = xTaskGetCurrentTaskHandle();
    taskHandles.flags = xEventGroupCreate();
    xTaskCreate(tasks::init::task, "init", 8192, &freematics, 16, &taskHandles.taskInit);
    while (!ulTaskNotifyTake(pdFALSE, 100));
    taskHandles.taskNet = tasks::create<tasks::net::NetworkTask>();
    while (!ulTaskNotifyTake(pdFALSE, 100));
    LOGI("System boot complete");
    Serial.println("\n\n");

    taskHandles.queueObd2Telem = xQueueCreate(configs::PID_LIST_LENGTH * 10, sizeof(sys::obd::PIDValue));
    taskHandles.queueTelem2Send = xQueueCreate(10, sizeof(char) * 256);
    taskHandles.queueGps2Telem = xQueueCreate(1, sizeof(::GPS_DATA));
    taskHandles.taskGps = tasks::create<tasks::gps::GpsTask>(&freematics);
    xTaskCreate(tasks::obd::task, "obd", 8192, freematics.link, 14, &taskHandles.taskObd);
    taskHandles.taskTelem = tasks::create<tasks::telem::TelemTask>();
    taskHandles.taskSend = tasks::create<tasks::send::SenderTask>();
    LOGI("System up and running");
    Serial.println("\n\n");

    while(true) {
        delay(30000);
    }
}

#if CONFIG_AUTOSTART_ARDUINO
void setup() { app(); }
void loop()  { delay(60000); }
#else
extern "C" void app_main() { app(); }
#endif
