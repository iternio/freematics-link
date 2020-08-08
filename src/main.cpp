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

//System initialization
bool initializeSystem(::FreematicsESP32& system);

//Helper functions (most likely to move to libraries)
void setTime(); //TODO: Move to library

//TODO: Make sure to properly clean up namespaces (including :: prefix to indicate global ns)
//Shared Resources (TODO: Should any of these be on a specific thread's stack instead of the global heap?)
::FreematicsESP32 freematics;
// sys::clt::HTTP* client;

void app() {
    Serial.begin(115200);
    // beep(880, 50);
    taskHandles.taskMain = xTaskGetCurrentTaskHandle();
    util::ptl();

    taskHandles.flags = xEventGroupCreate();

    util::ptl();
    xTaskCreate(tasks::init::task, "init", 8192, &freematics, 25, &taskHandles.taskInit);
    util::ptl();
    delay(200);
    util::ptl();
    delay(200);
    util::ptl();

    while (!ulTaskNotifyTake(pdFALSE, 100));

    util::ptl();
    // xTaskCreate(tasks::net::task, "net", 8192, NULL, 25, &taskHandles.taskNet);
    // xTaskCreate(tasks::run<tasks::net::NetworkTask>, "net", 8192, NULL, 25, &taskHandles.taskNet);
    // /* taskHandles.taskNet =  */tasks::test<tasks::net::NetworkTask>();
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

    // log_v("Initializing system");
    // ::FreematicsESP32 freematics;
    // initializeSystem(freematics);
    // log_v("Initialization complete");

    // log_v("Getting network connection");
    // sys::clt::HTTP* client = sys::net::getClient();
    // client = sys::net::getClient();

    // log_v("Setting system time");
    // setTime();

    // beep(880, 50, 2);

    taskHandles.queueObd2Telem = xQueueCreate(configs::PID_LIST_LENGTH * 10, sizeof(sys::obd::PIDValue));
    util::ptl();
    xTaskCreate(tasks::obd::task, "obd", 8192, freematics.link, 20, &taskHandles.taskObd);
    util::ptl();
    delay(200);
    util::ptl();
    delay(200);
    util::ptl();
    delay(600);
    util::ptl();
    xTaskCreate(tasks::telem::task, "telem", 8192, freematics.link, 15, &taskHandles.taskTelem);
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





// bool initializeSystem(::FreematicsESP32& system) {
//     //TODO: Move this to a library?
// #if !CONFIG_AUTOSTART_ARDUINO
//     log_v("Initializing Arduino system");
//     initArduino();
// #endif
//     log_v("Initializing Freematics system");
//     if (!system.begin()) {
//         log_e("Freematics system failed to initialize");
//         return false;
//     }
//     util::ptl();
//     printSysInfo(system);
//     util::ptl();
//     return true;
// }

// void setTime() {
//     //TODO: Sometimes this takes forever, figure out why, maybe stop & restart?
//     time_t now;
//     tm* nowtm;
//     char buf[35];
//     time(&now);
//     nowtm = gmtime(&now);
//     strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
//     log_v("Current system time: %s", buf);
//     log_v("Getting time via SNTP");
//     sntp_setoperatingmode(SNTP_OPMODE_POLL);
//     sntp_setservername(0, (char*)"pool.ntp.org");
//     sntp_init();
//     delay(500);
//     while (time(nullptr) < (2020 - 1970) * 365 * 24 * 3600) {
//         log_v("Waiting");
//         delay(1000);
//     }
//     // sntp_stop();
//     time(&now);
//     nowtm = gmtime(&now);
//     strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
//     log_v("System time updated to: %s", buf);
// }

#if CONFIG_AUTOSTART_ARDUINO
void setup() { app(); }
void loop()  { delay(60000); }
#else
extern "C" void app_main() { app(); }
#endif
