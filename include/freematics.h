/*
 * Wrapper for FreematicsPlus library, since it doesn't have an include guard
 */

#pragma once

#include <FreematicsPlus.h>

#include "util.h"

class Freematics : public FreematicsESP32 {
public:
    bool begin(bool useGNSS = true, bool useCellular = true) {
        Serial.println("NEW FREEMATICS CLASS BEGIN");
        bool ret = FreematicsESP32::begin(useGNSS, useCellular);
        link = new util::MutexLink(link);
        return ret;
    }
};
