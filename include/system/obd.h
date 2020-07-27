/*
 * Classes & functions to wrap Freematics OBD reading
 */

#pragma once

#include "freematics.h"

namespace sys {
    namespace obd {

        /* TODO: Some notes on drawbacks of the Freematics OBD implementation & potential things that need to be patched
        (Most of these seem like odd choices that are based on faulty assumptions of what "people need")
        - COBD hardcodes datamode = 1, assuming that all PIDs that will be read are mode 1.  datamode can be changed, but requires
            manually setting datamode member prior to calling readPID, rather than just passing in an optional datamode parameter to
            readPID
        - readPID assumes that data mode and PID in hex will onnly ever be 2 digits (is the OBD coprocessor limited by this, too?)
        - pidmap array of supported PIDs only checks from 0x00 - 0x80, which is only half of the mode 1 PIDs, what about higher
            PID values?  Above assumption about only mode 1 PIDs also applies, no PID map is checked for other modes, and there is
            no function to implement reading a mode's pidmap if datamode is set manually
        - the normalizeData function only seems to handle PIDs that have explicit #define constants, which consists of a subset of
            PIDs between 0x04 & 0x63 (again, the above assumption of only needing the most "common" PIDs)
        - Very little of COBD was defined as virtual, so almost none of it can be extended without having to deal with hiding
        - Reading PIDs calls idleTasks, but the default is just a 5ms delay, which seems not a lot of time to do "idle tasks" and
            kinda seems pointless, even if there is some down time while waiting for an OBD response
        */

       struct PID {
           uint16_t mode;
           uint16_t id;
           char name[16];
           char unit[8];
        //    char type;
           char header[4];
           char formula[24];
        //    uint32_t divisor;
       };

        struct PIDValue {
            PID pid;
            char raw[10];
            uint32_t value;
        };

        class OBD : public ::COBD {
        public:
            OBD_STATES state();
        };

        /* Parsed from the following lines in autopi-link:
        'soc':        "220,105,({32}/2.0),7E4",
        'soh':        "220,105,({us:26:27})/10.0,7E4",
        'voltage':    "220,101,({us:13:14})/10.0,7E4",
        'current':    "220,101,({s:11:12})/10.0,7E4",
        'is_charging':"220,101,int(not {51:2} and {10:0}),7E4",
        'ext_temp':   "220,100,({7}/2.0)-40.0,7B3",
        'batt_temp':  "220,101,{s:17},7E4",
        'odometer':   "22,B002,bytes_to_int(message.data[10:12]),7C6",
        'speed':      "220,100,bytes_to_int(message.data[32:33]),7B3",
        'kwh_charged':"220,101,(bytes_to_int(message.data[41:45]))/10.0,7E4",
        A few questions:
        - Why do the last three directly use the python bytes_to_int function rather than
          encoding them using the {} notation as the rest?
        - Why message.data[32:33] rather than just message.data[32]?
        - In parsing things, why are multi-byte ones parsed as bytes_to_int(data[1]) * 256 +
          bytes_to_int(data[2]) rather than just bytes_to_int(data[1:2]) when bytes_to_int
          will already hadnle the multiply by 256?
        - Why not just use bit shifting to parse the characters to integers?
        - There's ambiguity in some cases, like {2:3}, ie, is that an unsigned 16-bit int or bit 3 of byte 2?
        */

        const PID PID_LIST_EMULATOR[] = {
            {0x1,   0x43, "soc",              "%",        "7E4", "{us:1:2}*100.0/255.0"},
            {0x1,   0x0C, "voltage",          "V",        "7E4", "{us:1:2}/4.0"},
            {0x1,   0x05, "current",          "A",        "7E4", "{1}-40.0"},
            {0x1,   0x0C, "charge_voltage",   "V",        "7E4", "{us:1:2}/4.0"},
            {0x1,   0x5C, "charge_current",   "A",        "7E4", "{1}-40.0"},
            {0x1,   0x0E, "is_charging",      "",         "7E4", "{1}"},
            {0x1,   0x0D, "speed",            "km/h",     "7E4", "{1}"},
        };
        const uint8_t PID_LIST_EMULATOR_LENGTH = sizeof(PID_LIST_EMULATOR) / sizeof(PID);

        const PID PID_LIST_HYUNDAI_KONA[] = {
            {0x220, 0x105, "soh",              "%",        "7E4", "{32}/2.0"},
            {0x220, 0x105, "soc",              "%",        "7E4", "{us:26:27}/10.0"},
            {0x220, 0x101, "voltage",          "V",        "7E4", "{us:13:14}/10.0"},
            {0x220, 0x101, "current",          "A",        "7E4", "{s:11:12}/10.0"},
            {0x220, 0x101, "is_charging",      "",         "7E4", "!{51:2}&&{10:0}"},
            {0x220, 0x100, "ext_temp",         "\xB0""C",  "7B3", "{7}/2.0"},
            {0x220, 0x101, "batt_temp",        "\xB0""C",  "7E4", "{s:17}/2.0"},//Should this really be signed?
            {0x22B, 0x002, "odometer",         "km",       "7C6", "{us:10:11}/2.0"},
            {0x220, 0x100, "speed",            "km/h",     "7B3", "{30}/2.0"},
            {0x220, 0x101, "kwh_charged",      "kWh",      "7E4", "{41:44}/2.0"}
        };
        const uint8_t PID_LIST_HYUNDAI_KONA_LENGTH = sizeof(PID_LIST_HYUNDAI_KONA) / sizeof(PID);

    }
}
