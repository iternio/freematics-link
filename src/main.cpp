// #include "freematics.h"
#include <FreematicsPlus.h>
// #include <ArduinoJson.h>
#ifdef BOARD_HAS_PSRAM
#include <esp_himem.h>
#endif

// #include "mems.h"
// #include "abrpwifi.h"
// #include "util.h"
#include "abrp.h"
#include "config.h"

#define WIFI_SSID "------"
#define WIFI_PASSWORD "------"

#define LOOP_TIME 1000

FreematicsESP32 sys;

float randfloat(float min, float max, float dec = 1) {
    return random((int)((max - min) / dec) + 1) * dec + min;
}

float round(float val, float dec = 1) {
   return (int)(val / dec + 0.5) * dec;
}

void blink(uint32_t ms = 50, byte n = 1)
{
    static bool setup = false;
    if (!setup)
    {
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
}

void printSysInfo(const FreematicsESP32 &sys)
{
    log_i("CPU: %u MHz", ESP.getCpuFreqMHz());
    log_i("Flash: %u B", ESP.getFlashChipSize());
    // log_i("Flash: %u B", spi_flash_get_chip_size());
#ifdef BOARD_HAS_PSRAM
    log_i("IRAM: %u B", ESP.getHeapSize());
    log_i("PSRAM: %u B", ESP.getPsramSize() + esp_himem_get_phys_size());
#endif
    int rtc = rtc_clk_slow_freq_get();
    if (rtc)
        log_i("RTC: %i", rtc);
    log_i("Firmware: R%i", sys.version);
}

void startWiFi() {
    log_v("Connecting to " WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        log_v("Waiting");
        delay(1000);
    }
    log_v("Connected");
}

ABRPTelemetry telem;

void setup()
{
    log_v("Beginning set up");
    // delay(250);
    Serial.begin(115200);
    blink(25);
    if (sys.begin()) {
        delay(300);
        blink(25, 3);
        printSysInfo(sys);
    }
    startWiFi();
    telem.utc = 1591385576l + millis() / 1000l;
    telem.soc = randfloat(0, 100, 1);
    telem.speed = randfloat(0, 160, 1);
    telem.lat = randfloat(-90, 90, 7);
    telem.lon = randfloat(-180, 180, 7);
    telem.is_charging = false;
    telem.soh = randfloat(0, 100, 1);
    Serial.println(telem.toJSON());
}

void loop()
{
    unsigned long t = millis();
    telem.utc = 1591385576l + millis() / 1000l;
    telem.soc = round(telem.soc() + randfloat(-0.5, 0.5, 0.1), 0.1);
    telem.speed = round(telem.speed() + randfloat(-5, 5, 0.1), 0.1);
    telem.lat = round(telem.lat() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    telem.lon = round(telem.lon() + randfloat(-0.1, 0.1, 0.0000001), 0.0000001);
    Serial.println(telem.toJSON());

#ifdef VERBOSE
    blink(100);
#endif
    if (millis() - t < LOOP_TIME)
        delay(LOOP_TIME - (millis() - t));
}
