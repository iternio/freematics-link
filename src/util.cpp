#define LOG_LOCAL_NAME "util"
#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_DEBUG
#include "log.h"

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
#include "tasks/common.h"



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

#if ABRP_VERBOSE
    TaskStatus_t listtaskstatus[30];
    uint8_t listtaskidx = 0, listtaskcount = 0;
    TickType_t listtaskticks = 0;
    void ptl(bool all) {
        listtaskticks = xTaskGetTickCount();
        listtaskcount = uxTaskGetSystemState(listtaskstatus, 30, NULL);
        LOGD("Ticks   \tHandle  \tName      \tS\tNo\tBP\tCP\tTime    \tHWM  \tCPU");
        for (listtaskidx = 0; listtaskidx < listtaskcount; listtaskidx++) {
            if (all || listtaskstatus[listtaskidx].xHandle == taskHandles.taskMain ||
                listtaskstatus[listtaskidx].xHandle == taskHandles.taskObd ||
                listtaskstatus[listtaskidx].xHandle == taskHandles.taskTelem ||
                listtaskstatus[listtaskidx].xHandle == taskHandles.taskNet ||
                listtaskstatus[listtaskidx].xHandle == taskHandles.taskInit ||
                listtaskstatus[listtaskidx].xHandle == taskHandles.taskSend) {
                LOGD("%8u\t%p\t%-10s\t%1u\t%2u\t%2u\t%2u\t%8u\t%5u\t%2d",
                    listtaskticks,
                    listtaskstatus[listtaskidx].xHandle,
                    listtaskstatus[listtaskidx].pcTaskName,
                    listtaskstatus[listtaskidx].eCurrentState,
                    listtaskstatus[listtaskidx].xTaskNumber,
                    listtaskstatus[listtaskidx].uxBasePriority,
                    listtaskstatus[listtaskidx].uxCurrentPriority,
                    listtaskstatus[listtaskidx].ulRunTimeCounter,
                    listtaskstatus[listtaskidx].usStackHighWaterMark,
                    (listtaskstatus[listtaskidx].xCoreID > 1 ? -1 : listtaskstatus[listtaskidx].xCoreID)
                );
            }
        }
    }
#else
    void ptl(bool all) {}
#endif

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
        LOGV("expression");
        return ternary();
    }

    long double FormulaParser::ternary() {
        // Process a ternary expression: <term> [ ? <term> : <term> ]
        LOGV("ternary");
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
        LOGV("logical");
        //TODO: Figure out logical & bitwise operations...
        return bitwise();
    }

    long double FormulaParser::bitwise() {
        // Process a bitwise expression: <term> [ ( | ; & ; ^ ) <term> ]
        LOGV("bitwise");
        //TODO: Figure out logical & bitwise operations...
        return comparison();
    }

    long double FormulaParser::comparison() {
        // Process a comparison expression: <term> [ ( == ; != ; <= ; >= ; < ; > ) <term> ]
        LOGV("comparison");
        //TODO: Figure out logical & bitwise operations...
        return shift();
    }

    long double FormulaParser::shift() {
        // Process a bitshift expression: <term> [ ( << ; >> ) <term> ]
        LOGV("shift");
        //TODO: Figure out logical & bitwise operations...
        return additive();
    }

    long double FormulaParser::additive() {
        // Process an add or subtract expression: <term> [ ( + ; - ) <term> ]
        LOGV("additive");
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
        LOGV("multipicative");
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
        LOGV("unary");
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
        LOGV("exponentiation");
        long double result = value();
        while (peek() == '^') {
            get();  // Consume ^
            result = pow(result, unary());
        }
        return result;
    }

    long double FormulaParser::value() {
        // Parse a direct value, either a number or a substitution, or kick back to the top of the list for a parenthetical expression
        LOGV("value");
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
        LOGV("substitution");
        get(); // consume opening {
        LOGV("%s",formula);
        char type = 0;
        if (peek() > '9') {
            type = get();
            while (get() != ':');
        }
        LOGV("%c", type ? type : ' ');
        // Assuming data is an array of hex characters (if it's raw bytes, fix this)
        int from = number(), to;
        LOGV("%u", from);
        if (get() == ':') { // consume closing } or : separator
            to = number();
            LOGV("%u", to);
            get(); // consume closing }
            if (!type && to < 8) {
                LOGV("1 bit");
                uint8_t val;
                std::from_chars(&data[from*2], &data[from*2+2], val, 16);
                LOGV("%u", val);
                return (val & (1 << to)) >> to;
            }
        } else {
            to = from + 1;
            LOGV("%u", to);
        }
        LOGV("%i", to - from);
        switch (to - from) {
        case 1: {
            LOGV("8 bit");
            uint8_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            LOGV("%u", val);
            if (type == 's')
                return -(int8_t)(~val+1);
            return val;
        }
        case 2: {
            LOGV("16 bit");
            uint16_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            LOGV("%u", val);
            if (type == 's')
                return -(int16_t)(~val+1);
            return val;
        }
        case 3:
        case 4: {
            LOGV("32 bit");
            uint32_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            LOGV("%u", val);
            if (type == 's')
                return -(int32_t)(~val+1);
            return val;
        }
        default: {
            LOGV("64 bit");
            uint64_t val;
            std::from_chars(&data[from*2], &data[to*2], val, 16);
            LOGV("%llu", val);
            if (type == 's')
                return -(int64_t)(~val+1);
            return val;
        }
        }
    }

    long double FormulaParser::number() {
        // Process a number constant: 0-9.* [ \. 0-9.* ]
        LOGV("number");
        long double result = get() - '0';
        while (peek() >= '0' && peek() <= '9')
            result = result * 10 + (get() - '0');
        if (peek() == '.') {
            get();  // consume .
            save();
            while (peek() >= '0' && peek() <= '9')
                result += (get() - '0') / pow(10, recall());
        }
        LOGV("%Lf", result);
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

    MutexLink::MutexLink(CLink * l) :
        link(l),
        mutex(xSemaphoreCreateRecursiveMutex()),
        n(0) {
        LOGD("Created Mutexed CLink");
    }

    MutexLink::~MutexLink() {
        vSemaphoreDelete(mutex);
        delete link;
    }

    void MutexLink::take() {
        //TODO: Should we actually just block forever?  Or should there be a timeout failure mechanism?
        LOGV("Taking (%p) - %u", xTaskGetCurrentTaskHandle(), n);
        xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
        LOGV("Taken (%p) - %u", xTaskGetCurrentTaskHandle(), ++n);
    }

    void MutexLink::give() {
        LOGV("Giving (%p) - %u", xTaskGetCurrentTaskHandle(), n);
        xSemaphoreGiveRecursive(mutex);
        LOGV("Given (%p) - %u", xTaskGetCurrentTaskHandle(), --n);
    }

    int MutexLink::read() {
        take();
        int ret = link->read();
        give();
        return ret;
    }

    bool MutexLink::send(const char * str) {
        take();
        bool ret = link->send(str);
        give();
        return ret;
    }

    int MutexLink::receive(char * buffer, int bufsize, unsigned int timeout) {
        take();
        int ret = link->receive(buffer, bufsize, timeout);
        give();
        return ret;
    }

    int MutexLink::sendCommand(const char * cmd, char * buf, int bufsize, unsigned int timeout) {
        take();
        int ret = link->sendCommand(cmd, buf, bufsize, timeout);
        give();
        return ret;
    }

    MutexLink::operator bool() const {
        return (bool)link;
    }


}
