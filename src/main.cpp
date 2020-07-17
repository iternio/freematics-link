#include <FreematicsPlus.h>
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

using namespace util;

//System initialization
bool initializeSystem(::FreematicsESP32& system);

//Task functions
void taskInitialize(void* taskToNotify);
void taskReadTelemetry(void* taskToNotify);
void taskSendTelemetry(void* taskToNotify);

//Helper functions (most likely to move to libraries)
void setTime(); //TODO: Move to library
void connectObd() {};   //TODO: Move to a library
abrp::telemetry::Telemetry getTelemetry();    //TODO: Pass in OBD connection
bool isDriving(abrp::telemetry::Telemetry& telem) { return true; }; //TODO: Move to library
bool isCharging(abrp::telemetry::Telemetry& telem) { return false; }; //TODO: Move to library
bool sendTelemetry(sys::clt::HTTP& client, abrp::telemetry::Telemetry& telem);  //TODO: Move to library

char taskbuffer[1024];

#if ABRP_VERBOSE
void printTaskList() {
    vTaskList(taskbuffer);
    log_i("Task Info:\nTask Name\tStatus\tPrio\tHWM\tTask\tAffinity\n%s", taskbuffer);
}
#else
#define printTaskList()
#endif


//TODO: move away from Arduino style and just write a main, can avoid globals!
::FreematicsESP32 freematics;
sys::clt::HTTP* client;
// abrp::telemetry::Telemetry telem;

void app() {
    Serial.begin(115200);
    beep(880, 50);
    log_v("System boot complete");
    printTaskList();
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

    beep(880, 50, 2);

    log_v("Setting up telemetry reader task");
    QueueHandle_t telemQ = xQueueCreate(1, sizeof(abrp::telemetry::Telemetry));
    TaskHandle_t readTask, sendTask;
    xTaskCreate(taskReadTelemetry, "reader", 8192, telemQ, 10, &readTask);
    xTaskCreate(taskSendTelemetry, "sender", 8192, telemQ, 15, &sendTask);

    printTaskList();
    log_v("Beginning main loop");

    while(true) {
        delay(30000);
        printTaskList();
    }
}

void taskReadTelemetry(void* telemetryQ) {
    log_v("Getting OBD connection");
    connectObd();   //TODO: Actually return an OBD connection
    abrp::telemetry::Telemetry telem;
    TickType_t last = xTaskGetTickCount();
    const TickType_t rate = pdMS_TO_TICKS(configs::RATE_OBD_READ);
    while (true) {
        telem = getTelemetry();  //TODO: Pass in OBD connection
        if (isDriving(telem) || isCharging(telem)) {
            log_v("Telemetry ready");
            xQueueOverwrite(telemetryQ, &telem);
        }
        log_d("Telem task sleeping");
        vTaskDelayUntil(&last, rate);
        log_d("Telem task running");
    }
}

void taskSendTelemetry(void* telemetryQ) {
    abrp::telemetry::Telemetry telem;
    TickType_t last = xTaskGetTickCount();
    const TickType_t rate = pdMS_TO_TICKS(configs::RATE_TELEM_SEND);
    while (true) {
        log_d("Send task waiting for data");
        if (xQueueReceive(telemetryQ, &telem, pdMS_TO_TICKS(configs::RATE_TELEM_SEND)) &&
            Serial.println() &&
            sendTelemetry(*client, telem)) {
            log_v("Telemetry sent");
            Serial.println();
        } else {
            log_v("Failed to send telemetry");
#if ABRP_VERBOSE>1
            beep(3000, 10, 3);
#endif
        }
        log_d("Send task sleeping");
        vTaskDelayUntil(&last, rate);
        log_d("Send task running");
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
    printTaskList();
    printSysInfo(system);
    printTaskList();
    return true;
}

abrp::telemetry::Telemetry getTelemetry() {
    //TODO: Move this to a library
    static abrp::telemetry::Telemetry telem;
    static bool init = false;
    if  (!init) {
        telem.utc = time(nullptr);
        telem.soc = randfloat(0, 100, 1);
        telem.speed = randfloat(0, 160, 1);
        telem.lat = randfloat(-90, 90, 0.001);
        telem.lon = randfloat(-180, 180, 0.001);
        telem.is_charging = false;
        telem.soh = randfloat(0, 100, 1);
        telem.power = randfloat(-20, 20, 0.01);
        init = true;
    }
    telem.utc = time(nullptr);
    telem.soc = round(telem.soc() + randfloat(-0.5, 0.5, 0.1), 0.1);
    telem.speed = round(telem.speed() + randfloat(-5, 5, 0.1), 0.1);
    telem.lat = round(telem.lat() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    telem.lon = round(telem.lon() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    telem.power = round(telem.power() + randfloat(-2, 2, 0.01), 0.01);
    return telem;
}

bool sendTelemetry(sys::clt::HTTP& client, abrp::telemetry::Telemetry& telem) {
    static bool init = false;
    static String telemstr;
    if (!init) {
        char url[50], authkey[50];
        strcpy(url, abrp::params::PROTOCOL);
        strcat(url, abrp::params::HOST);
        strcat(url, abrp::params::SEND_ENDPOINT);
        strcpy(authkey, abrp::params::HEADER_AUTH_TEXT);
        strcat(authkey, configs::APIKEY);
        client.setUrl(url);
        client.urlParams.set(abrp::params::VAR_TOKEN, configs::TOKEN);
        client.reqHeaders.set(abrp::params::HEADER_AUTH, authkey);
        init = true;
    }
    telemstr = telem.toJSON();
    log_v("Telem: %s", telemstr.c_str());
    client.urlParams.set(abrp::params::VAR_TELEM, telemstr);
    return client.get();
}

void setTime() {
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
