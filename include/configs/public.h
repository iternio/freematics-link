/*
 * Header defining program configuration paramgers
 */

#pragma once

#include "system/obd.h"


#define USE_PID_LIST(LIST_NAME) \
        inline const sys::obd::PID (& PID_LIST)[sys::obd::PID_LIST_ ##LIST_NAME ##_LENGTH] = sys::obd::PID_LIST_##LIST_NAME; \
        inline const uint8_t & PID_LIST_LENGTH = sys::obd::PID_LIST_ ##LIST_NAME ##_LENGTH; \
        inline const char (& PID_LIST_NAME)[sizeof(sys::obd::PID_LIST_ ##LIST_NAME ##_NAME)/sizeof(char)] = sys::obd::PID_LIST_ ##LIST_NAME ##_NAME;


namespace configs {

    inline const short LOOP_TIME = 10000;  //How often to repeat main loop, ms
    inline const short RATE_OBD_READ = 10000;
    inline const short RATE_TELEM_CONVERT = 0;
    inline const short RATE_TELEM_SEND = 20000;

    USE_PID_LIST(EMULATOR);

}
