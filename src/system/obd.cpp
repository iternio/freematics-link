/*
 * Extensions to Freematics library's OBD client
 */

#include "system/obd.h"

#include "freematics.h"

namespace sys {
    namespace obd {

        uint8_t OBD::readPIDRaw(uint8_t mode, uint16_t pid, char * buffer, uint8_t bufsize) {
            sprintf(buffer, (pid > 0xFF ? "%02X%04X\r" : "%02X%02X\r"), mode, pid);
            //TODO: mutex for link usage!
            link->send(buffer);
            // delay(5); //TODO: This is where the original implementation calls idleTasks(), which seems unecessary here.  Should we delay? It's not clear if receive will block with a delay, or just spin
            int ret = link->receive(buffer, bufsize, OBD_TIMEOUT_SHORT);
            if (ret > 0 && checkErrorMessage(buffer)) {
                log_d("Received error message %s", buffer);
                errors++;
                return 0;
            }
            return ret;
        }

        uint8_t OBD::readPIDRaw(uint16_t pid, char * buffer, uint8_t bufsize) {
            return readPIDRaw(0x01, pid, buffer, bufsize);
        }

        bool OBD::normalizePIDFromFormula(char * data, uint8_t datalen, char * formula) {
            //Parse value tokens
            // double values[10];
            // char * c1, c2;



            return false;
        }

        OBD_STATES OBD::state() {
            return m_state;
        }

    }
}
