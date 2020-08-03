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
        // char buffer[1024];
        // vTaskList(buffer);
        // log_i("Task Info:\nTask Name\tStatus\tPrio\tHWM\tTask\tAffinity\n%s", buffer);
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

    /* Parser should observer the following order of operations:
        ()              parentheses
        ^               exponentiation
        -               unary negation
        ~               unary bitwise not
        !               unary logical not
        * / %           multiplicative
        + -             additive
        >> <<           bitwise shift
        < > <= >= == != comparison
        & | ^           bitwise operations
        && ||           logical operations
        ?:              ternary operator
    */

    long double FormulaParser::expression() {
        // Process an expression starting at the bottom of the above list
        // log_d("expression");
        return ternary();
    }

    long double FormulaParser::ternary() {
        // Process a ternary expression: <term> [ ? <term> : <term> ]
        // log_d("ternary");
        long double condition = logical();
        if (peek() != '?')
            return condition;
        get(); // Consume ?
        if (condition) {
            long double result = logical();
            get(); // Consume :
            logical(); // Consume false value (no designated end delimiter, just have to process it)
            return result;
        }
        logical(); // Consume true value (can't just look for :, as data substitutions use it too)
        get(); // Consume :
        return logical();
    }

    long double FormulaParser::logical() {
        // Process a logical expression: <term> [ ( || ; && ) <term> ]
        // log_d("logical");
        //TODO: Figure out logical & bitwise operations...
        return bitwise();
    }

    long double FormulaParser::bitwise() {
        // Process a bitwise expression: <term> [ ( | ; & ; ^ ) <term> ]
        // log_d("bitwise");
        //TODO: Figure out logical & bitwise operations...
        return comparison();
    }

    long double FormulaParser::comparison() {
        // Process a comparison expression: <term> [ ( == ; != ; <= ; >= ; < ; > ) <term> ]
        // log_d("comparison");
        //TODO: Figure out logical & bitwise operations...
        return shift();
    }

    long double FormulaParser::shift() {
        // Process a bitshift expression: <term> [ ( << ; >> ) <term> ]
        // log_d("shift");
        //TODO: Figure out logical & bitwise operations...
        return additive();
    }

    long double FormulaParser::additive() {
        // Process an add or subtract expression: <term> [ ( + ; - ) <term> ]
        // log_d("additive");
        long double result = multipicative();
        while (true) {
            switch (peek()) {
            case '+':
                get();  // Consume +
                result += multipicative();
            case '-':
                get();  // Consume -
                result -= multipicative();
            default:
                return result;
            }
        }
    }

    long double FormulaParser::multipicative() {
        // Process a multiply or divide expression: <term> [ ( * ; / ; % ) <term> ]
        // log_d("multipicative");
        long double result = unary();
        while (true) {
            switch (peek()) {
            case '*':
                get();  // Consume *
                result *= unary();
            case '/':
                get();  // Consume /
                result /= unary();
            // Figure out modulus
            // case '%':
            //     get();  // Consume %
            //     return result % unary();
            default:
                return result;
            }
        }
    }

    long double FormulaParser::unary() {
        // Process a unary operator expression: [ ( - ; ! ; ~ ) ] <term>
        // log_d("unary");
        switch (peek()) {
            case '-':
                get(); // Consume -
                return -unary();
            //TODO: Figure out logical & bitwise operations...
            // case '!':
            //     get(); // Consume !
            //     return !unary();
            // case '~':
            //     get(); //Consume '~'
            //     return ~unary();
            default:
                return exponentiation();
        }
    }

    long double FormulaParser::exponentiation() {
        // Process an exponent expression: <term> [ ^ <term> ]
        // log_d("exponentiation");
        long double result = value();
        while (peek() == '^') {
            get();  // Consume ^
            result = pow(result, unary());
        }
        return result;
    }

    long double FormulaParser::value() {
        // Parse a direct value, either a number or a substitution, or kick back to the top of the list for a parenthetical expression
        // log_d("value");
        if (peek() >= '0' && peek() <= '9') {
            // Value is a number constant
            return number();
        } else if (peek() == '{') {
            // Value is a data substitution
            return substitution();
        } else if (peek() == '(') {
            // Value is a parenthetical expression
            get(); // consume opening (
            long double result = expression();
            get(); // consume closing )
            return result;
        }
        return 0; // Error
    }

    long double FormulaParser::substitution() {
        // Process a data substution { [ ( u ; s ) : ] 0-9.* [ : 0-9.* ] }
        // log_d("substitution");
        get(); // consume opening {
        // log_d("%s",formula);
        char type = 0;
        if (peek() > '9') {
            type = get();
            while (get() != ':');
        }
        // log_d("%c", type ? type : ' ');
        // Assuming data is an array of hex characters (if it's raw bytes, fix this)
        int from = number(), to;
        // log_d("%u", from);
        if (get() == ':') { // consume closing } or : separator
            to = number();
            // log_d("%u", to);
            get(); // consume closing }
            if (!type && to < 8) {
                // log_d("1 bit");
                uint8_t val;
                std::from_chars(&data[from*2], &data[from*2+2], val, 16);
                // log_d("%u", val);
                return (val & (1 << to)) >> to;
            }
        } else {
            to = from + 1;
            // log_d("%u", to);
        }
        // log_d("%i", to - from);
        switch (to - from) {
        case 1: {
            // log_d("8 bit");
            uint8_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            // log_d("%u", val);
            if (type == 's')
                return -(int8_t)(~val+1);
            return val;
        }
        case 2: {
            // log_d("16 bit");
            uint16_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            // log_d("%u", val);
            if (type == 's')
                return -(int16_t)(~val+1);
            return val;
        }
        case 3:
        case 4: {
            // log_d("32 bit");
            uint32_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            // log_d("%u", val);
            if (type == 's')
                return -(int32_t)(~val+1);
            return val;
        }
        default: {
            // log_d("64 bit");
            uint64_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            // log_d("%u", val);
            if (type == 's')
                return -(int64_t)(~val+1);
            return val;
        }
        }
    }

    long double FormulaParser::number() {
        // Process a number constant: 0-9.* [ \. 0-9.* ]
        // log_d("number");
        long double result = get() - '0';
        while (peek() >= '0' && peek() <= '9')
            result = result * 10 + (get() - '0');
        if (peek() == '.') {
            get();  // consume .
            save();
            while (peek() >= '0' && peek() <= '9')
                result += (get() - '0') / pow(10, recall());
        }
        // log_d("%Lf", result);
        return result;
    }

    //----------------------------

    char FormulaParser::get() {
        //Return current character and advance to next
        while (*formula == ' ' || *formula == '\t' || *formula == '\r' || *formula == '\n') { // Consume white space
            saved ++;   // Saved is primarily used to count characters, so it shouldn't count ignored white space
            formula++;
        }
        return *formula++;
    }

    char FormulaParser::peek() {
        //Return current character only
        return *formula;
    }

    void FormulaParser::save() {
        //Save current position for later comparison
        saved = formula;
    }

    uint8_t FormulaParser::recall() {
        //Return number of characters processed since last saved
        return formula - saved;
    }

}
