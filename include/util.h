#ifndef INC_UTIL_H
#define INC_UTIL_H

// #include "freematics.h"
#include <stdint.h>

class FreematicsESP32;

void blink(uint32_t ms = 50, byte n = 1);
void beep(uint32_t freq = 2000, uint32_t ms = 50, byte n = 1);

void printSysInfo(const FreematicsESP32& sys);

#endif
