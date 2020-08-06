/*
 * Header wrapping network interfaces of Freematics in a standard interface
 */

#pragma once

// #include <Client.h>
// #include <string.h>

#include "http.h"
#include "networks/common.h"
#include "networks/wifi.h"
#include "networks/bt.h"
#include "networks/sim.h"

namespace sys {
    namespace net {

        enum NetworkPriorities {
            NETWORK_WIFI,
            NETWORK_WIFI_SECONDARY,
            NETWORK_BT,
            NETWORK_SIM,
            NETWORK_COUNT
        };

        clt::HTTP* getClient();

    }
}
