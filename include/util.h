/*
 * Helpful utility functions
 */


#pragma once

#include <stdint.h>

class FreematicsESP32;

namespace util {

    float randfloat(float min, float max, float dec = 1);
    float round(float val, float dec = 1);
    void blink(uint32_t ms = 50, byte n = 1);
    void beep(uint32_t freq = 2000, uint32_t ms = 50, byte n = 1);

    void printSysInfo(const FreematicsESP32& sys);

}
