#include <FreematicsPlus.h>
#ifdef BOARD_HAS_PSRAM
#ifdef CONFIG_USING_ESPIDF
#include <esp32/himem.h>
#else
#include <esp_himem.h>
#endif
#endif
#include "util.h"

namespace util {

    float randfloat(float min, float max, float dec) {
        return random((long)(min / dec), (long)(max / dec + 1)) * dec;
    }

    float round(float val, float dec) {
        return (int)(val / dec + 0.5) * dec;
    }

    void blink(uint32_t ms, byte n) {
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

    void beep(uint32_t freq, uint32_t ms, byte n) {
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

}
