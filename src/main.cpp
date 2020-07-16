// #include "freematics.h"
#include <FreematicsPlus.h>
// #include <ArduinoJson.h>
#ifdef BOARD_HAS_PSRAM
#ifdef CONFIG_USING_ESPIDF
#include <esp32/himem.h>
#else
#include <esp_himem.h>
#endif
#endif
#include <lwip/apps/sntp.h>
//#include <string.h>

#include "abrp.h"
#include "configs.h"
#include "system.h"
#include "util.h"

String test = "blah";

// namespace abrp {

using namespace util;

// void startWiFi();

// void doSetup(); //TODO: This is a temporary function as I build out the full capabilities
// void doLoop();  //TODO: This is a temporary function as I build out the full capabilities

bool initializeSystem(::FreematicsESP32& system);   //TODO: Move to library
void setTime(); //TODO: Move to library
void connectObd() {};   //TODO: Move to a library
abrp::telemetry::Telemetry getTelemetry();    //TODO: Pass in OBD connection
bool isDriving(abrp::telemetry::Telemetry& telem) { return true; }; //TODO: Move to library
bool isCharging(abrp::telemetry::Telemetry& telem) { return false; }; //TODO: Move to library
bool sendTelemetry(sys::clt::HTTP& client, abrp::telemetry::Telemetry& telem);  //TODO: Move to library

// FreematicsESP32 sys;
// abrp::telemetry::Telemetry telem;
// abrp::clients::HTTP c;
// WiFiClient w;

// void setup() {
//     doSetup();
// }

// void loop() {
//     doLoop();
// }

//TODO: move away from Arduino style and just write a main, can avoid globals!
::FreematicsESP32 syst;
sys::clt::HTTP* client;
abrp::telemetry::Telemetry telem;

void app() {
// void setup() {
    Serial.begin(115200);
    beep(880, 50);
    log_v("System boot");

    log_v("Initializing system");
    // ::FreematicsESP32 system;
    initializeSystem(syst);
    log_v("Initialization complete");

    log_v("Getting network connection");
    // sys::clt::HTTP* client = sys::net::getClient();
    client = sys::net::getClient();

    log_v("Setting system time");
    setTime();

    log_v("Getting OBD connection");
    connectObd();   //TODO: Actually return an OBD connection
    beep(880, 50, 2);

    log_v("Beginning main loop");
    // abrp::telemetry::Telemetry telem;
// }
// void loop() {
    unsigned long t;
    while(true) {
        t = millis();
        Serial.println();
#ifdef VERBOSE
        blink(100);
#endif
        telem = getTelemetry();  //TODO: Pass in OBD connection
        if (isDriving(telem) || isCharging(telem)) {
            if (!sendTelemetry(*client, telem)) {
#ifdef VERBOSE
                beep(3000, 10, 3);
#endif
            }
        }

        if (millis() - t < configs::LOOP_TIME)
            delay(configs::LOOP_TIME - (millis() - t));
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
    printSysInfo(system);
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

// void doSetup() {
    // log_v("Boot complete");
    // beep(880, 50);
    // delay(200);

    // log_v("Beginning set up");
    // Serial.begin(115200);
    // blink(25);
    // if (!sys.begin()) {
    //     log_e("Failed to initialize system!");
    //     return;
    // }
    // delay(300);
    // blink(25, 3);

    // printSysInfo(sys);
    // Serial.println();

    // startWiFi();
    // Serial.println();

    // setTime();
    // Serial.println();

    // char url[50], authkey[50];
    // strcpy(url, abrp::params::PROTOCOL);
    // strcat(url, abrp::params::HOST);
    // strcat(url, abrp::params::SEND_ENDPOINT);
    // strcpy(authkey, abrp::params::HEADER_AUTH_TEXT);
    // strcat(authkey, configs::APIKEY);
    // // c.configure(w, url);
    // c.urlParams.set(abrp::params::VAR_TOKEN, configs::TOKEN);
    // c.reqHeaders.set(abrp::params::HEADER_AUTH, authkey);

    // telem.utc = time(nullptr);
    // telem.soc = randfloat(0, 100, 1);
    // telem.speed = randfloat(0, 160, 1);
    // telem.lat = randfloat(-90, 90, 0.001);
    // telem.lon = randfloat(-180, 180, 0.001);
    // telem.is_charging = false;
    // telem.soh = randfloat(0, 100, 1);
    // log_v("Starting telem: %s", telem.toJSON().c_str());

    // c.urlParams.set(abrp::params::VAR_TELEM, telem.toJSON());
    // c.get();

    // log_v("Set Up Complete");
    // beep(880, 50, 2);
// }

// String telemstr;

// void doLoop() {
    // Serial.println();
    // log_v("Main Loop");
    // unsigned long t = millis();

// #ifdef VERBOSE
//     blink(100);
// #endif

    // telem.utc = time(nullptr);
    // telem.soc = round(telem.soc() + randfloat(-0.5, 0.5, 0.1), 0.1);
    // telem.speed = round(telem.speed() + randfloat(-5, 5, 0.1), 0.1);
    // telem.lat = round(telem.lat() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    // telem.lon = round(telem.lon() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    // telemstr = telem.toJSON();
    // log_v("Telem: %s", telemstr.c_str());

    // c.urlParams.set(abrp::params::VAR_TELEM, telemstr);

// #ifdef VERBOSE
//     if (!c.get())
//         beep(3000, 10, 3);
// #endif

//     if (millis() - t < configs::LOOP_TIME)
//         delay(configs::LOOP_TIME - (millis() - t));
// }

// void startWiFi() {
//     log_v("Connecting to %s", configs::SSID);
//     while (WiFi.status() != WL_CONNECTED) {
//         log_v("Trying to connect");
//         WiFi.begin(configs::SSID, configs::PASSWORD);
//         for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED; i ++) {
//             log_v("Waiting (%u)", WiFi.status());
//             delay(500);
//         }
//         if (WiFi.status() == WL_CONNECT_FAILED) {
//             log_v("Failed to connect");
//             delay(2000);
//         }
//     }
//     log_v("Connected");
// }

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

// }

#if CONFIG_AUTOSTART_ARDUINO
void setup() { app(); }
void loop()  {}
#else
extern "C" void app_main() { app(); }
#endif
