/*
 * Task to access PIDs via OBD connection
 */

#pragma once

namespace tasks {
    namespace obd {

        void task(void* param);
        bool init();

    }
}
