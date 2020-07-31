#include "freematics.h"
#ifdef BOARD_HAS_PSRAM
#ifdef CONFIG_USING_ESPIDF
#include <esp32/himem.h>
#else
#include <esp_himem.h>
#endif
#endif
#include <charconv>
#include <cmath>
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
        //TODO: I'm pretty sure this board is supposed to have an RTC, figure this out
        int rtc = rtc_clk_slow_freq_get();
        if (rtc)
            log_i("RTC: %i", rtc);
        log_i("Firmware: R%i", sys.version);
        log_i("Flash: %u B", ESP.getFlashChipSize());
        log_i("Flash (SPI): %u B", spi_flash_get_chip_size());
        log_i("IRAM: %u B", ESP.getHeapSize());
    #ifdef BOARD_HAS_PSRAM
        log_i("PSRAM: %u B", ESP.getPsramSize() + esp_himem_get_phys_size());
        log_i("SPIRAM: %u B", ESP.getPsramSize());
        log_i("HiMem Phys Size: %u B", esp_himem_get_phys_size());
        log_i("HiMem Reserved: %u B", esp_himem_reserved_area_size());
    #endif
        // log_i("Heap info:");
        // heap_caps_print_heap_info(MALLOC_CAP_EXEC);
        // heap_caps_print_heap_info(MALLOC_CAP_32BIT);
        // heap_caps_print_heap_info(MALLOC_CAP_8BIT);
        // heap_caps_print_heap_info(MALLOC_CAP_DMA);
        // heap_caps_print_heap_info(MALLOC_CAP_PID2);
        // heap_caps_print_heap_info(MALLOC_CAP_PID3);
        // heap_caps_print_heap_info(MALLOC_CAP_PID4);
        // heap_caps_print_heap_info(MALLOC_CAP_PID5);
        // heap_caps_print_heap_info(MALLOC_CAP_PID6);
        // heap_caps_print_heap_info(MALLOC_CAP_PID7);
        // heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
        // heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
        // heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
        // heap_caps_print_heap_info(MALLOC_CAP_INVALID);
        // heap_caps_dump_all();
        char buffer[1024];
        vTaskList(buffer);
        log_i("Task Info:\nTask Name\tStatus\tPrio\tHWM\tTask\tAffinity\n%s", buffer);
    }

    uint16_t str_remove(char * s, const char * c = " ") {
        char * from = s;
        char * to = s;
        //TODO: Should this remove just ' ' or also '\n' or '\r' or '\t' (or have flags for these)
        while (*from) {
            if (to != from)
                *to = *from;
            from++;
            to++;
            while (*from && strchr(c, *from))
                from++;
        }
        *to = '\0';
        return strlen(s);
    }

    //The below parser generally assumes the formula is well formed and free of syntax errors
    //If it's not, it likely won't cough, but will just return invalid results
    long double FormulaParser::parse(const char * formulaptr, const char * dataptr) {
        formula = formulaptr;
        saved = formula;
        data = dataptr;
        return expression();
    }

    long double FormulaParser::number() {
        double result = get() - '0';
        while (peek() >= '0' && peek() <= '9')
            result = result * 10 + (get() - '0');
        if (peek() == '.') {
            get();  // consume .
            save();
            while (peek() >= '0' && peek() <= '9')
                result += (get() - '0') / pow(10, recall());
        }
        return result;
    }

    long double FormulaParser::substitution() {
        get(); // consume opening {
        char type = 0;
        if (peek() > '9') {
            type = get();
            while (get() != ':');
        }
        //Assuming data is an array of hex characters (if it's raw bytes, fix this)
        int from = number(), to;
        if (get() == ':') { // consume closing } or : separator
            to = number();
            get(); // consume closing }
            if (!type && to < 8) {
                uint8_t val;
                std::from_chars(&data[from*2], &data[from*2+2], val, 16);
                return (val & (1 << to)) >> to;
            }
        } else {
            to = from + 1;
        }
        switch (from - to) {
        case 1: {
            uint8_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            if (type == 's')
                return -(int8_t)(~val+1);
            return val;
        }
        case 2: {
            uint16_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            if (type == 's')
                return -(int16_t)(~val+1);
            return val;
        }
        case 3:
        case 4: {
            uint32_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            if (type == 's')
                return -(int32_t)(~val+1);
            return val;
        }
        default: {
            uint64_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            if (type == 's')
                return -(int64_t)(~val+1);
            return val;
        }
        }
    }

    long double FormulaParser::factor() {
        if (peek() >= '0' && peek() <= '9')
            return number();
        else if (peek() == '(') {
            get(); // consume opening (
            int result = expression();
            get(); // consume closing )
            return result;
        } else if (peek() == '-') {
            get();  // consume -
            return -factor();
        } else if (peek() == '{') {
            return substitution();
        }
        return 0; // error
    }

    long double FormulaParser::term() {
        long double result = factor();
        while (peek() == '*' || peek() == '/')
            if (get() == '*')
                result *= factor();
            else
                result /= factor();
        return result;
    }

    long double FormulaParser::expression() {
        long double result = term();
        while (peek() == '+' || peek() == '-')
            if (get() == '+')
                result += term();
            else
                result -= term();
        return result;
    }

    char FormulaParser::get() {
        return *formula++;
    }

    char FormulaParser::peek() {
        return *formula;
    }

    void FormulaParser::save() {
        saved = formula;
    }

    uint8_t FormulaParser::recall() {
        return formula - saved;
    }

}
