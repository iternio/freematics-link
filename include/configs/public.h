/*
 * Header defining program configuration paramgers
 */

#pragma once

#include "system/obd.h"

namespace configs {

    inline const short LOOP_TIME = 10000;  //How often to repeat main loop, ms
    inline const short RATE_OBD_READ = 10000;
    inline const short RATE_TELEM_SEND = 20000;

    // const sys::obd::PID PID_LIST[sys::obd::PID_LIST_EMULATOR_LENGTH] = sys::obd::PID_LIST_EMULATOR;
    inline const sys::obd::PID* const PID_LIST_PTR = sys::obd::PID_LIST_EMULATOR;
    inline const uint8_t PID_LIST_LENGTH = sys::obd::PID_LIST_EMULATOR_LENGTH;

}
