/*
 * Task to maintain network connections
 */

#include "freematics.h"
#include <Client.h>

#include "tasks/common.h"
#include "tasks/net.h"
#include "configs.h"
#include "system/network.h"

namespace tasks {
    namespace net {

        bool findWiFiNetwork(const char * const ssid) {
            uint16_t numfound = WiFi.scanNetworks();
            char * found = 0;
            for (uint16_t i = 0; i < numfound; i ++) {
                found = (char *)((wifi_ap_record_t *)WiFi.getScanInfoByIndex(i))->ssid;
                log_v("Nework %u: %s", i, found);
                if (strcmp(found, ssid) == 0)
                    return true;
            }
            return false;
        }

        bool connectToWiFi(const char * const ssid, const char * const password) {
            uint8_t tries = 0;
            bool found = false;
            const uint8_t maxtries = 3; //TODO: Make configurable
            log_v("Attempting to connect to WiFi network %s", ssid);
            while (!(WiFi.isConnected() && strcmp(WiFi.SSID().c_str(), ssid) == 0) && tries++ < maxtries) {
                if (!found) {
                    log_v("Looking for networks");
                    if (!findWiFiNetwork(ssid)) {
                        log_v("Network not found");
                        delay(1000);
                        log_v("Retrying");
                        continue;
                    }
                    found = true;
                }
                log_v("Network found");
                WiFi.begin(ssid, password);
                WiFi.waitForConnectResult();
                if (!WiFi.isConnected() && tries < maxtries) {
                    log_v("Failed to connect");
                    delay(5000);
                    log_v("Retrying");
                }
            }
            if (!(WiFi.isConnected() && strcmp(WiFi.SSID().c_str(), ssid) == 0)) {
                log_v("Failed to connect to %s", ssid);
                return false;
            }
            log_v("Connected to %s", ssid);
            return true;
        }

        bool connectToSim() {
            log_d("Cell connection not implemented");
            return false;
        }

        bool connectToBluetooth() {
            log_e("Bluetooth connection not implemented");
            return false;
        }

        uint8_t tryConnections(const sys::net::NetworkPriorities * const priorities, const uint8_t max) {
            log_v("Stepping through connection priorities up to %u", max);
            for (uint8_t i = 0; i < max; i ++) {
                log_v("Trying priority %u - %u", i, priorities[i]);
                switch(priorities[i]) {
                case sys::net::NETWORK_WIFI:
                    log_v("Trying to connect to primary WiFI network");
                    if (connectToWiFi(configs::SSID, configs::PASSWORD))
                        return i;
                    break;
                case sys::net::NETWORK_WIFI_SECONDARY:
                    log_v("Trying to connect to secondary WiFI network");
                    if (connectToWiFi(configs::SSID_SECONDARY, configs::PASSWORD_SECONDARY))
                        return i;
                    break;
                case sys::net::NETWORK_SIM:
                    log_v("Trying to connect to cellular network");
                    //TODO: Impelement SimClient
                    if (connectToSim())
                        return i;
                    break;
                case sys::net::NETWORK_BT:
                    log_v("Trying to connect to Bluetooth device");
                    //TODO: Implement BluetoothClient
                    if (connectToBluetooth())
                        return i;
                    break;
                default:
                    log_v("Unknown type of network");
                }
            }
            return UINT8_MAX;
        }

        void task(void * param) {
            //TODO: do more than just start the wifi initially
            //This task is intended to constantly monitor the intenret connection
            //And reestablish it if it's lost, or upgrade it if a higher priority
            //Becomes available
            // sys::clt::HTTP * client = sys::net::getClient();

            //Set flag that internet is connected
            // xEventGroupSetBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK | tasks::FLAG_NETWORK_IS_WIFI);

            // while (true)
            //     delay(60000);

            /* Psuedocode for future set up of this function:
            while (true) {
                while (checkForInternetConnection())
                    delay(30000);   //TODO: Figure out reasonable sleep time
                for (connType in connTypesInPriorityOrder) {
                    if (connectTo(connType))
                        break;
                }
            }
            */

            //TODO: Look at using events to react quicker to loss of network

            //TODO: Make network priorities, test server/port, recheck rate, upgrade check rate configurable
            const sys::net::NetworkPriorities priorities[] = {sys::net::NETWORK_WIFI, sys::net::NETWORK_WIFI_SECONDARY, sys::net::NETWORK_SIM};   //TODO: Make this a constant in configs
            const uint8_t numPriorities = sizeof(priorities) / sizeof(sys::net::NetworkPriorities);
            const char * const testServer = "api.iternio.com";
            const uint16_t testPort = 80;
            uint8_t currentType = UINT8_MAX, lastType = UINT8_MAX;
            uint8_t checkUpTo = numPriorities;
            uint32_t checkConnectionRate = 30000;    //TODO: Do this like the others with a config define & task loop functions instead of delay
            uint8_t tryUpgradeCycles = 4;
            uint8_t nextUpgradeTry = tryUpgradeCycles;
            // uint8_t tries = 0;
            // const char * ssid = 0, * password = 0;
            ::Client * network = NULL;

            while (true) {
                //Check for existing network connection w/ valid connection, or
                //TODO: Actually try to connect to a website here...
                checkUpTo = numPriorities;
                log_v("Checking for internet connection");
                while (network && (network->connected() || network->connect(testServer, testPort))) {
                    log_v("Internet connection found");
                    if (currentType > 0 && nextUpgradeTry-- <= 0) {
                        checkUpTo = currentType;
                        log_v("Checking if can upgrade to higher priority connection");
                        break;
                    }
                    delay(checkConnectionRate);
                    log_v("Checking for internet connection");
                }
                log_v("Trying to connect to internet");
                nextUpgradeTry = tryUpgradeCycles;
                lastType = currentType;
                currentType = tryConnections(priorities, checkUpTo);
                if (currentType > numPriorities && checkUpTo == lastType) {
                    currentType = lastType;
                    log_v("Did not find higher priority connection");
                    continue;
                }
                if (currentType == lastType) {
                    log_v("Reconnected via same network");
                    continue;
                }
                delete network;
                network = NULL;
                xEventGroupClearBits(taskHandles.flags, tasks::FLAG_NETWORK_IS_WIFI | tasks::FLAG_NETWORK_IS_SIM | tasks::FLAG_NETWORK_IS_BT);
                switch (priorities[currentType]) {
                case sys::net::NETWORK_WIFI:
                case sys::net::NETWORK_WIFI_SECONDARY:
                    network = new WiFiClient;
                    xEventGroupSetBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK | tasks::FLAG_NETWORK_IS_WIFI);
                    break;
                case sys::net::NETWORK_SIM:
                    //TODO: Impelement SimClient
                    log_v("Now connected via cell network");
                    xEventGroupSetBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK | tasks::FLAG_NETWORK_IS_SIM);
                    break;
                case sys::net::NETWORK_BT:
                    //TODO: Implement BluetoothClient
                    log_v("Now connected via Bluetooth");
                    xEventGroupSetBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK | tasks::FLAG_NETWORK_IS_BT);
                    break;
                default:
                    log_v("Could not find network connection");
                    xEventGroupClearBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK);
                }
            }
        }

    }
}
