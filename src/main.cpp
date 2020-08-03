#include "freematics.h"
#ifdef BOARD_HAS_PSRAM
#ifdef CONFIG_USING_ESPIDF
#include <esp32/himem.h>
#else
#include <esp_himem.h>
#endif
#endif
#include <lwip/apps/sntp.h>

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

#if ABRP_VERBOSE
TaskStatus_t listtaskstatus[30];
uint8_t listtaskidx = 0, listtaskcount = 0;
TickType_t listtaskticks = 0;
void ptl() {
    listtaskticks = xTaskGetTickCount();
    listtaskcount = uxTaskGetSystemState(listtaskstatus, 30, NULL);
    log_d("Ticks   \tHandle  \tName      \tS\tNo\tBP\tCP\tTime    \tHWM  \tCPU");
    for (listtaskidx = 0; listtaskidx < listtaskcount; listtaskidx++) {
        if (listtaskstatus[listtaskidx].xHandle == taskHandles.taskMain ||
            listtaskstatus[listtaskidx].xHandle == taskHandles.taskObd ||
            listtaskstatus[listtaskidx].xHandle == taskHandles.taskTelem) {
            log_d("%8u\t%08X\t%-10s\t%1u\t%2u\t%2u\t%2u\t%8lu\t%5u\t%2i",
                listtaskticks,
                listtaskstatus[listtaskidx].xHandle,
                listtaskstatus[listtaskidx].pcTaskName,
                listtaskstatus[listtaskidx].eCurrentState,
                listtaskstatus[listtaskidx].xTaskNumber,
                listtaskstatus[listtaskidx].uxBasePriority,
                listtaskstatus[listtaskidx].uxCurrentPriority,
                listtaskstatus[listtaskidx].ulRunTimeCounter,
                listtaskstatus[listtaskidx].usStackHighWaterMark,
                listtaskstatus[listtaskidx].xCoreID
            );
        }
    }
}
#else
#define ptl()
#endif

//Shared Resources (TODO: Should any of these be on a specific thread's stack instead of the global heap?)
::FreematicsESP32 freematics;
sys::clt::HTTP* client;

void app() {
    Serial.begin(115200);
    taskHandles.taskMain = xTaskGetCurrentTaskHandle();
    // beep(880, 50);
    log_v("System boot complete");
    ptl();
    Serial.println();

    log_v("Initializing system");
    // ::FreematicsESP32 freematics;
    initializeSystem(freematics);
    log_v("Initialization complete");

    log_v("Getting network connection");
    // sys::clt::HTTP* client = sys::net::getClient();
    client = sys::net::getClient();

    log_v("Setting system time");
    setTime();

    // beep(880, 50, 2);

    taskHandles.queueObd2Telem = xQueueCreate(configs::PID_LIST_LENGTH * 10, sizeof(sys::obd::PIDValue));
    ptl();
    xTaskCreate(tasks::obd::task, "obd", 8192, freematics.link, 20, &taskHandles.taskObd);
    ptl();
    delay(200);
    ptl();
    delay(200);
    ptl();
    delay(600);
    ptl();
    xTaskCreate(tasks::telem::task, "telem", 8192, freematics.link, 15, &taskHandles.taskTelem);
    ptl();
    delay(200);
    ptl();
    delay(200);
    ptl();

    while(true) {
        ptl();
        delay(30000);
    }
}





bool initializeSystem(::FreematicsESP32& system) {
    //TODO: Move this to a library?
#if !CONFIG_AUTOSTART_ARDUINO
    log_v("Initializing Arduino system");
    initArduino();
#endif
    log_v("Initializing Freematics system");
    if (!system.begin()) {
        log_e("Freematics system failed to initialize");
        return false;
    }
    ptl();
    printSysInfo(system);
    ptl();
    return true;
}

void setTime() {
    //TODO: Sometimes this takes forever, figure out why, maybe stop & restart?
    time_t now;
    tm* nowtm;
    char buf[35];
    time(&now);
    nowtm = gmtime(&now);
    strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
    log_v("Current system time: %s", buf);
    log_v("Getting time via SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*)"pool.ntp.org");
    sntp_init();
    delay(500);
    while (time(nullptr) < (2020 - 1970) * 365 * 24 * 3600) {
        log_v("Waiting");
        delay(1000);
    }
    // sntp_stop();
    time(&now);
    nowtm = gmtime(&now);
    strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
    log_v("System time updated to: %s", buf);
}

#if CONFIG_AUTOSTART_ARDUINO
void setup() { app(); }
void loop()  {}
#else
extern "C" void app_main() { app(); }
#endif
