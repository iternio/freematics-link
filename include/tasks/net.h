/*
 * Classes & functions to wrap maintaining network connction
 */

#pragma once

#include "freematics.h"
#include "tasks/common.h"
#include "system/network.h"

namespace tasks {
    namespace net {

        // void task(void* param);

        class NetworkTask : public Task {
        public:
            NetworkTask(void * p);
            void run();

        private:
            bool findWiFiNetwork(const char * const ssid);
            bool connectToWiFiNetwork(const char * const ssid, const char * const password);
            bool connectToCellNetwork();
            bool connectToBluetoothDevice();
            bool tryConnections();
            bool checkInternetConnection();

            ::Client * network;
            bool hasNetwork;
            uint8_t currentNetwork;
            bool hasInternet;
            Freematics * system;

        public:
            static const sys::net::NetworkPriorities priorities[];
            static const uint8_t numPriorities;
            static const uint32_t checkConnectionRate;
            static const uint32_t noConnectionRate;
            static const uint8_t tryUpgradeCycles;

            static const char name[];
            static const uint32_t memory;
            static const uint8_t priority;
        };

    }
}
