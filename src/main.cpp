// #include "freematics.h"
#include <FreematicsPlus.h>
// #include <ArduinoJson.h>
#ifdef BOARD_HAS_PSRAM
#include <esp_himem.h>
#endif
#include <lwip/apps/sntp.h>

#include "util.h"

#include "abrp/abrp.h"
#include "config/config.h"
#include "config/private.h"

using namespace util;

FreematicsESP32 sys;

void startWiFi() {
    log_v("Connecting to %s", config::SSID);
    while (WiFi.status() != WL_CONNECTED) {
        log_v("Trying to connect");
        WiFi.begin(config::SSID, config::PASSWORD);
        for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED; i ++) {
            log_v("Waiting (%u)", WiFi.status());
            delay(500);
        }
        if (WiFi.status() == WL_CONNECT_FAILED) {
            log_v("Failed to connect");
            delay(2000);
        }
    }
    log_v("Connected");
}

void time_sync_notification_cb(struct timeval *tv)
{
    log_v("Notification of a time synchronization event");
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

// MyHTTP::MyClient ABRPclient;

// void connectABRP() {
//     ABRPclient.begin(
//         "https://" ABRP_SERVER_HOST ABRP_SERVER_ENDPOINT "?"
//             ABRP_SERVER_TOKENVAR "=" ABRP_SERVER_TOKEN,
//         ABRP_SERVER_CACERT);
// }

// void sendABRP(const String &tlm) {
//     String payload =
//         "--ABR1234PABRP1234ABRP\n"
//         "Content-Disposition: form-data; name=\"tlm\"\n\n"
//         + tlm + "\n"
//         "--ABR1234PABRP1234ABRP--";
//     ABRPclient.addHeader(ABRP_SERVER_AUTHHEADER, ABRP_SERVER_AUTHTEXT ABRP_SERVER_APIKEY);
//     ABRPclient.addHeader("Content-Type", "multipart/form-data; boundary=\"ABR1234PABRP1234ABRP\"");
//     ABRPclient.post(payload);
//     log_v("%s", "Sent");
//     WiFiClient stream = ABRPclient.getStream();
//     while (stream.available()) {
//         char ch = static_cast<char>(stream.read());
//         Serial.print(ch);
//     }
//     Serial.println();
//     log_v("%s", "Done");
//     Serial.println();
//     Serial.println();
//     Serial.println();
// }

abrp::telemetry::Telemetry telem;
abrp::clients::HTTP c;
WiFiClient w;

void setup()
{
    log_v("Boot complete");
    beep(880, 50);
    delay(200);

    log_v("Beginning set up");
    Serial.begin(115200);
    blink(25);
    if (!sys.begin()) {
        log_e("Failed to initialize system!");
        return;
    }
    delay(300);
    blink(25, 3);

    printSysInfo(sys);
    Serial.println();

    startWiFi();
    Serial.println();

    setTime();
    Serial.println();

    char url[50], authkey[50];
    strcpy(url, abrp::params::PROTOCOL);
    strcat(url, abrp::params::HOST);
    strcat(url, abrp::params::SEND_ENDPOINT);
    strcpy(authkey, abrp::params::HEADER_AUTH_TEXT);
    strcat(authkey, config::APIKEY);
    c.configure(w, url);
    c.urlParams.set(abrp::params::VAR_TOKEN, config::TOKEN);
    c.reqHeaders.set(abrp::params::HEADER_AUTH, authkey);

    telem.utc = time(nullptr);
    telem.soc = randfloat(0, 100, 1);
    telem.speed = randfloat(0, 160, 1);
    telem.lat = randfloat(-90, 90, 0.001);
    telem.lon = randfloat(-180, 180, 0.001);
    telem.is_charging = false;
    telem.soh = randfloat(0, 100, 1);
    log_v("Starting telem: %s", telem.toJSON().c_str());

    // c.urlParams.set(abrp::params::VAR_TELEM, telem.toJSON());
    // c.get();

    log_v("Set Up Complete");
    beep(880, 50, 2);
}

String telemstr;

void loop()
{
    Serial.println();
    // log_v("Main Loop");
    unsigned long t = millis();

#ifdef VERBOSE
    blink(100);
#endif

    telem.utc = time(nullptr);
    telem.soc = round(telem.soc() + randfloat(-0.5, 0.5, 0.1), 0.1);
    telem.speed = round(telem.speed() + randfloat(-5, 5, 0.1), 0.1);
    telem.lat = round(telem.lat() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    telem.lon = round(telem.lon() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    telemstr = telem.toJSON();
    log_v("Telem: %s", telemstr.c_str());

    c.urlParams.set(abrp::params::VAR_TELEM, telemstr);

#ifdef VERBOSE
    if (!c.get())
        beep(3000, 10, 3);
#endif

    if (millis() - t < config::LOOP_TIME)
        delay(config::LOOP_TIME - (millis() - t));
}
