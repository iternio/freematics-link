/*
 * Classes & functions to wrap processing OBD data into ABRP telemetry
 */

#pragma once

namespace tasks {
    namespace telem {

        void task(void* param);
        bool init();

    }
}
