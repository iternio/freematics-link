// #include "freematics.h"
#include <FreematicsPlus.h>

#include "mems.h"
#include "util.h"

FreematicsESP32 sys;

void setup()
{
    delay(250);
    Serial.begin(115200);
    Serial.println("Set Up");
    blink(25);
    if (sys.begin()) {
        delay(300);
        blink(25, 3);
        printSysInfo(sys);
    }
}

void loop()
{
    Serial.println("Loop");
    delay(900);
#ifdef VERBOSE
    blink(100);
#else
    delay(100);
#endif
}
