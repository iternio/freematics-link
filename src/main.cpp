// #include "freematics.h"
#include <FreematicsPlus.h>
// #include <ArduinoJson.h>
#ifdef BOARD_HAS_PSRAM
#include <esp_himem.h>
#endif
#include <lwip/apps/sntp.h>
// #include "httpclientwrapper.h"

// #include "mems.h"
// #include "abrpwifi.h"
// #include "util.h"
#include "abrp.h"
// #include "config.h"

#include "abrp/abrp.h"
#include "config/config.h"
#include "config/private.h"

#define WIFI_SSID "Black Hole"
#define WIFI_PASSWORD "eventhorizon"

FreematicsESP32 sys;

float randfloat(float min, float max, float dec = 1) {
    return random((long)(min / dec), (long)(max / dec + 1)) * dec;
}

float round(float val, float dec = 1) {
   return (int)(val / dec + 0.5) * dec;
}

void blink(uint32_t ms = 50, byte n = 1)
{
    static bool setup = false;
    if (!setup) {
        pinMode(PIN_LED, OUTPUT);
        setup = true;
    }
    for (byte i = 0; i < n; i++) {
        digitalWrite(PIN_LED, HIGH);
        delay(ms);
        digitalWrite(PIN_LED, LOW);
        if (i < n-1)
            delay(ms);
    }
}

void beep(uint32_t freq = 2000, uint32_t ms = 50, byte n = 1)
{
    static bool setup = false;
    if (!setup) {
        ledcSetup(0, 2000, 8);
        ledcAttachPin(PIN_BUZZER, 0);
        setup = true;
    }
    ledcWriteTone(0, freq);
    for (byte i = 0; i < n; i++) {
        ledcWrite(0, 255);
        delay(ms);
        ledcWrite(0, 0);
        if (i < n-1)
            delay(ms);
    }
}

void printSysInfo(const FreematicsESP32 &sys)
{
    esp_chip_info_t cinfo;
    esp_chip_info(&cinfo);
    log_i("Chip: %s", cinfo.model == CHIP_ESP32 ? "ESP32" : "Unknown");
    log_i("Cores: %u", cinfo.cores);
    log_i("Rev: %u", cinfo.revision);
    log_i("Features: EMB %c, WiFi %c, BLE %c, BT %c",
        cinfo.features & CHIP_FEATURE_EMB_FLASH ? 'Y' : 'N',
        cinfo.features & CHIP_FEATURE_WIFI_BGN ? 'Y' : 'N',
        cinfo.features & CHIP_FEATURE_BLE ? 'Y' : 'N',
        cinfo.features & CHIP_FEATURE_BT ? 'Y' : 'N');
    log_i("CPU: %u MHz", ESP.getCpuFreqMHz());
    log_i("Flash: %u B", ESP.getFlashChipSize());
    // log_i("Flash: %u B", spi_flash_get_chip_size());
#ifdef BOARD_HAS_PSRAM
    log_i("IRAM: %u B", ESP.getHeapSize());
    log_i("PSRAM: %u B", ESP.getPsramSize() + esp_himem_get_phys_size());
#endif
    //TODO: I'm pretty sure this board is supposed to have an RTC, figure this out
    int rtc = rtc_clk_slow_freq_get();
    if (rtc)
        log_i("RTC: %i", rtc);
    log_i("Firmware: R%i", sys.version);
}

void startWiFi() {
    log_v("Connecting to " WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        log_v("Trying to connect");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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

ABRPTelemetry telem;
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
