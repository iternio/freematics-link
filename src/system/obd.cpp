/*
 * Extensions to Freematics library's OBD client
 */

#include "system/obd.h"

#include <FreematicsPlus.h>

namespace sys {
    namespace obd {

        OBD_STATES OBD::state() {
            return m_state;
        }

    }
}
