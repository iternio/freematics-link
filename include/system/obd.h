/*
 * Classes & functions to wrap Freematics OBD reading
 */

#include <FreematicsPlus.h>

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
           char type;
           char header[4];
           char formula[24];
           uint32_t divisor;
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

    }
}
