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
           uint8_t mode;
           uint16_t id;
           uint16_t header;
           char name[16];
           char unit[8];
        //    char type;
        //    char header[4];
           char formula[24];
        //    uint32_t divisor;
       };

        struct PIDValue {
            const PID & pid;    //TODO: Make this into a reference value?
            uint32_t sequence;
            char raw[128];
            long double value;
        };

        class OBD : public ::COBD {
        public:
            OBD_STATES state();
            uint8_t readPIDRaw(uint8_t mode, uint16_t pid, char * buffer, uint8_t bufsize);
            uint8_t readPIDRaw(uint16_t pid, char * buffer, uint8_t bufsize);
            bool normalizePIDFromFormula(char * data, uint8_t datalen, char * formula);
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

        //TODO: should I try to differentiate between regular and extended PIDs?  Right now, a regular PID just has 0xFF extid
        inline const PID PID_LIST_EMULATOR[] = {
            {0x01,   0x43,  0x7E8,  "soc",              "%",     "{us:0:2}*100.0/255.0"},
            {0x01,   0x0C,  0x7E8,  "voltage",          "V",     "{us:0:2}/4.0"},
            {0x01,   0x05,  0x7E8,  "current",          "A",     "{0}-40.0"},
            {0x01,   0x0C,  0x7E8,  "charge_voltage",   "V",     "{us:0:2}/4.0"},
            {0x01,   0x5C,  0x7E8,  "charge_current",   "A",     "{0}-40.0"},
            {0x01,   0x0E,  0x7E8,  "is_charging",      "",      "{0:1}"},
            {0x01,   0x0D,  0x7E8,  "speed",            "km/h",  "{0}"},
        };
        inline const uint8_t PID_LIST_EMULATOR_LENGTH = sizeof(PID_LIST_EMULATOR) / sizeof(PID);
        inline const char PID_LIST_EMULATOR_NAME[] = "Emulator";

        inline const PID PID_LIST_HYUNDAI_KONA[] = {
            {0x22, 0x0105,  0x7E4,  "soh",              "%",         "{32}/2.0"},
            {0x22, 0x0105,  0x7E4,  "soc",              "%",         "{us:26:27}/10.0"},
            {0x22, 0x0101,  0x7E4,  "voltage",          "V",         "{us:13:14}/10.0"},
            {0x22, 0x0101,  0x7E4,  "current",          "A",         "{s:11:12}/10.0"},
            {0x22, 0x0101,  0x7E4,  "is_charging",      "",          "!{51:2}&&{10:0}"},
            {0x22, 0x0100,  0x7B3,  "ext_temp",         "\xB0""C",   "{7}/2.0"},
            {0x22, 0x0101,  0x7E4,  "batt_temp",        "\xB0""C",   "{s:17}/2.0"},//Should this really be signed?
            {0x22, 0xB002,  0x7C6,  "odometer",         "km",        "{us:10:11}/2.0"},
            {0x22, 0x0100,  0x7B3,  "speed",            "km/h",      "{30}/2.0"},
            {0x22, 0x0101,  0x7E4,  "kwh_charged",      "kWh",       "{41:44}/2.0"}
        };
        inline const uint8_t PID_LIST_HYUNDAI_KONA_LENGTH = sizeof(PID_LIST_HYUNDAI_KONA) / sizeof(PID);
        inline const char PID_LIST_HYUNDAI_KONA_NAME[] = "Hyundai Kona";

        // inline const PID* const PID_LIST_KIA_NIRO = PID_LIST_HYUNDAI_KONA;
        inline const PID (& PID_LIST_KIA_NIRO)[PID_LIST_HYUNDAI_KONA_LENGTH] = PID_LIST_HYUNDAI_KONA;
        inline const uint8_t & PID_LIST_KIA_NIRO_LENGTH = PID_LIST_HYUNDAI_KONA_LENGTH;
        inline const char PID_LIST_KIA_NIRO_NAME[] = "Kia Niro";

    }
}
