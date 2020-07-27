/*
 * Extensions to Freematics library's OBD client
 */

#include "system/obd.h"

#include "freematics.h"

namespace sys {
    namespace obd {

        OBD_STATES OBD::state() {
            return m_state;
        }

    }
}
