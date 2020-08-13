/*
 * Extensions to Freematics library's OBD client
 */

#define LOG_LOCAL_NAME "obd"
#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#include "log.h"

#include "system/obd.h"

#include "freematics.h"
#include <charconv>
#include "util.h"



namespace sys {
    namespace obd {

        uint8_t OBD::readPIDRaw(uint8_t mode, uint16_t pid, char * buffer, uint8_t bufsize) {
            sprintf(buffer, (pid > 0xFF ? "%02X%04X\r" : "%02X%02X\r"), mode, pid);
            //TODO: mutex for link usage!
            LOGD("Sending: %s", buffer);
            link->send(buffer);
            // delay(5); //TODO: This is where the original implementation calls idleTasks(), which seems unecessary here.  Should we delay? It's not clear if receive will block with a delay, or just spin
            int ret = link->receive(buffer, bufsize, OBD_TIMEOUT_SHORT);
            LOGD("Received: %s", buffer);
            if (ret > 0 && checkErrorMessage(buffer)) {
                LOGW("Received error message %s", buffer);
                errors++;
                return 0;
            }
            return ret;
        }

        uint8_t OBD::readPIDRaw(uint16_t pid, char * buffer, uint8_t bufsize) {
            return readPIDRaw(0x01, pid, buffer, bufsize);
        }

        char * OBD::parseRaw(char * raw, const PID & pid, uint16_t * header, uint8_t * size, uint8_t * mode, uint16_t * id) {
            char * p = raw;
            uint16_t val;
            if (header)
                *header = 0;
            if (mode)
                *mode = 0;
            if (size)
                *size = 0;
            if (id)
                *id = 0;
            //Clean up raw data
            util::str_remove(raw, " \r\n>");
            //Check responder header matches expectation
            std::from_chars(p, p + 3, val, 16);
            if (header)
                *header = val;
            p += 3;
            if (val != pid.header) {
                LOGW("Header mismatch: %03X != %03X", *header, pid.header);
                return 0;
            }
            //Check given size matches actual data
            std::from_chars(p, p + 2, val, 16);
            if (size)
                *size = val;
            p += 2;
            if (val != strlen(p) / 2) {
                LOGW("Size mismatch: %u != %u", *size, strlen(p) / 2);
                return 0;
            }
            //Check response mode matches expectation
            std::from_chars(p, p + 2, val, 16);
            if (mode)
                *mode = val;
            size -= 1;
            p += 2;
            if (val != pid.mode + 0x40) {
                LOGW("Mode mismatch: %02X != %02X", *mode, pid.mode + 0x40);
                return 0;
            }
            //Do all manufacturer specific higher order mode PIDs have 2 byte ids?  This check may need to be more advanced
            std::from_chars(p, p + (pid.mode > 0x09 && pid.id > 0xFF ? 4 : 2), val, 16);
            if (id)
                *id = val;
            size -= (pid.mode > 0x09 && pid.id > 0xFF ? 2 : 1);
            p += (pid.mode > 0x09 && pid.id > 0xFF ? 4 : 2);
            if (val != pid.id) {
                LOGW("ID mismatch: %.*X != %.*X", (pid.mode > 0x09 && pid.id > 0xFF ? 4 : 2), *id,
                    (pid.mode > 0x09 && pid.id > 0xFF ? 4 : 2), pid.id);
                return 0;
            }
            return p;
        }

        long double OBD::normalizePIDFromFormula(const char * formula, const char * data) {
            static util::FormulaParser parser;
            LOGV("%s <-- %s", formula, data);
            return parser.parse(formula, data);
        }

        OBD_STATES OBD::state() {
            return m_state;
        }

    }
}
