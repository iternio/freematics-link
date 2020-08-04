/*
 * Task to maintain network connections
 */

#include "freematics.h"

#include "tasks/common.h"
#include "tasks/net.h"
#include "configs.h"
#include "system/network.h"

namespace tasks {
    namespace net {

        void task(void * param) {
            //TODO: do more than just start the wifi initially
            //This task is intended to constantly monitor the intenret connection
            //And reestablish it if it's lost, or upgrade it if a higher priority
            //Becomes available
            sys::clt::HTTP * client = sys::net::getClient();

            //Set flag that internet is connected
            xEventGroupSetBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK | tasks::FLAG_NETWORK_IS_WIFI);

            while (true)
                delay(60000);
        }

    }
}
