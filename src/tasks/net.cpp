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

        // class NetworkTask {
        // public:
        //     NetworkTask();
        //     void run();

        // private:
        //     bool findWiFiNetwork(const char * const ssid);
        //     bool connectToWiFiNetwork(const char * const ssid, const char * const password);
        //     bool connectToCellNetwork();
        //     bool connectToBluetoothDevice();
        //     bool tryConnections();
        //     bool checkInternetConnection();

        //     ::Client * network;
        //     bool hasNetwork;
        //     uint8_t currentNetwork;
        //     bool hasInternet;

        // public:
        //     static const sys::net::NetworkPriorities priorities[];
        //     static const uint8_t numPriorities;
        //     static const uint32_t checkConnectionRate;
        //     static const uint32_t noConnectionRate;
        //     static const uint8_t tryUpgradeCycles;
        //     static const bool deleteSelfWhenDone;
        // };

        const char NetworkTask::name[] = "net";
        const uint32_t NetworkTask::memory = 6144;
        const uint8_t NetworkTask::priority = 20;

        //TODO: Make these constants in configs
        const sys::net::NetworkPriorities NetworkTask::priorities[] = {sys::net::NETWORK_WIFI, sys::net::NETWORK_WIFI_SECONDARY, sys::net::NETWORK_SIM};
        const uint8_t NetworkTask::numPriorities = sizeof(NetworkTask::priorities) / sizeof(sys::net::NetworkPriorities);
        const uint32_t NetworkTask::checkConnectionRate = 30000;
        const uint32_t NetworkTask::noConnectionRate = 10000;
        const uint8_t NetworkTask::tryUpgradeCycles = 4;
        // const bool NetworkTask::deleteSelfWhenDone = false;

        NetworkTask::NetworkTask(void * p) : Task(p), network(NULL), hasNetwork(false), currentNetwork(UINT8_MAX), hasInternet(false) {}

        bool NetworkTask::findWiFiNetwork(const char * const ssid) {
            log_d("A %u %u", WiFi.status(), WiFi.isConnected());
            //This is a workaround.  If scanNetworks is called while the wifi task is still attempting to reconect
            //to a lost connection, the CPU will panic.  It's supposed to just fail and return a warning, but for
            //some reason doesn't.  If we still have a current network connection to a valid AP, the scan will work,
            //so we don't have to kill the current network to try to find a better one.  But if we've lost our
            //connection, and the system doesn't properly report as fully disconnected, it is likely still trying to
            //reconnect, so we should call disconnect to make sure it stops before scanning for networks (hopefully)
            if ((hasNetwork && WiFi.status() != WL_CONNECTED) ||
                (!hasNetwork && WiFi.status() != WL_DISCONNECTED)) {
                log_v("First, disconnecting from WiFi");
                WiFi.disconnect();
                log_d("B %u %u", WiFi.status(), WiFi.isConnected());
                delay(500);
                log_d("C %u %u", WiFi.status(), WiFi.isConnected());
            }
            log_v("Searching for WiFi networks");
            uint16_t numfound = WiFi.scanNetworks();
            log_d("D %u %u", WiFi.status(), WiFi.isConnected());
            char * found = 0;
            for (uint16_t i = 0; i < numfound; i ++) {
                found = (char *)((wifi_ap_record_t *)WiFi.getScanInfoByIndex(i))->ssid;
                log_v("Nework %u: %s", i, found);
                if (strcmp(found, ssid) == 0)
                    return true;
            }
            log_d("E %u %u", WiFi.status(), WiFi.isConnected());
            return false;
        }

        bool NetworkTask::connectToWiFiNetwork(const char * const ssid, const char * const password) {
            uint8_t tries = 0;
            bool found = false;
            const uint8_t maxtries = 3; //TODO: Make configurable
            log_v("Attempting to connect to WiFi network %s", ssid);
            while (!(WiFi.isConnected() && strcmp(WiFi.SSID().c_str(), ssid) == 0) && tries++ < maxtries) {
                log_d("1 %u %u", WiFi.status(), WiFi.isConnected());
                if (!found) {
                    log_v("Looking for networks");
                    if (!findWiFiNetwork(ssid)) {
                        log_d("2 %u %u", WiFi.status(), WiFi.isConnected());
                        log_v("Network not found");
                        delay(1000);
                        log_v("Retrying");
                        continue;
                    }
                    log_d("3 %u %u", WiFi.status(), WiFi.isConnected());
                    found = true;
                    log_v("Network found, trying to connect");
                    WiFi.disconnect();
                    log_d("4 %u %u", WiFi.status(), WiFi.isConnected());
                    delay(500);
                    log_d("5 %u %u", WiFi.status(), WiFi.isConnected());
                }
                log_d("6 %u %u", WiFi.status(), WiFi.isConnected());
                WiFi.begin(ssid, password);
                log_d("7 %u %u", WiFi.status(), WiFi.isConnected());
                WiFi.waitForConnectResult();
                log_d("8 %u %u", WiFi.status(), WiFi.isConnected());
                if (!(WiFi.isConnected() && strcmp(WiFi.SSID().c_str(), ssid) == 0) && tries < maxtries) {
                    log_d("9 %u %u", WiFi.status(), WiFi.isConnected());
                    log_v("Failed to connect");
                    delay(5000);
                    log_v("Retrying");
                    log_d("0 %u %u", WiFi.status(), WiFi.isConnected());
                }
            }
            if (!(WiFi.isConnected() && strcmp(WiFi.SSID().c_str(), ssid) == 0)) {
                log_v("Failed to connect to %s", ssid);
                return false;
            }
            log_v("Connected to %s", ssid);
            return true;
        }

        bool NetworkTask::connectToCellNetwork() {
            log_d("Cell connection not implemented");
            return false;
        }

        bool NetworkTask::connectToBluetoothDevice() {
            log_e("Bluetooth connection not implemented");
            return false;
        }

        bool NetworkTask::tryConnections() {
            uint8_t max = (hasNetwork ? currentNetwork : numPriorities);
            log_v("Stepping through connection priorities up to index %u", max);
            for (uint8_t i = 0; i < max; i ++) {
                log_v("Trying priority index %u: %u", i, priorities[i]);
                switch(priorities[i]) {
                case sys::net::NETWORK_WIFI:
                    log_v("Trying to connect to primary WiFI network");
                    if (!connectToWiFiNetwork(configs::SSID, configs::PASSWORD))
                        continue;
                    break;
                case sys::net::NETWORK_WIFI_SECONDARY:
                    log_v("Trying to connect to secondary WiFI network");
                    if (!connectToWiFiNetwork(configs::SSID_SECONDARY, configs::PASSWORD_SECONDARY))
                        continue;
                    break;
                case sys::net::NETWORK_SIM:
                    log_v("Trying to connect to cellular network");
                    //TODO: Impelement SimClient
                    if (!connectToCellNetwork())
                        continue;
                    break;
                case sys::net::NETWORK_BT:
                    log_v("Trying to connect to Bluetooth device");
                    //TODO: Implement BluetoothClient
                    if (!connectToBluetoothDevice())
                        continue;
                    break;
                default:
                    log_v("Unknown type of network");
                    continue;
                }
                currentNetwork = i;
                return true;
            }
            return false;
        }

        bool NetworkTask::checkInternetConnection() {
            // const char * const testServer = "api.iternio.com";
            // const uint16_t testPort = 80;
            log_v("Checking for internet connection");
            if (!network) {
                log_v("No network to try");
                return false;
            }
            if (network->connected()) {
                log_v("Client is already connected");
                return true;
            }
            if (network->connect("api.iternio.com", 80)) {
                log_v("Connected to the internet successfully");
                return true;
            }
            log_v("Could not connect to the internet");
            return false;
        }

        void NetworkTask::run() {
            //TODO: Look at using events to react quicker to loss of network
            //TODO: Make network priorities, test server/port, recheck rate, upgrade check rate configurable
            // const sys::net::NetworkPriorities priorities[] = {sys::net::NETWORK_WIFI, sys::net::NETWORK_WIFI_SECONDARY, sys::net::NETWORK_SIM};   //TODO: Make this a constant in configs
            // const uint8_t numPriorities = sizeof(priorities) / sizeof(sys::net::NetworkPriorities);

            // uint8_t currentType = UINT8_MAX, lastType = UINT8_MAX;
            // uint8_t checkUpTo = numPriorities;
            // uint32_t checkConnectionRate = 30000;    //TODO: Do this like the others with a config define & task loop functions instead of delay
            // uint8_t tryUpgradeCycles = 4;
            // uint8_t nextUpgradeTry = tryUpgradeCycles;
            // ::Client * network = NULL;

            while (true) {
                //Check for an existing internet conection, go back to sleep if one exists (every so often, check for a better connection)
                for (uint8_t i = 0; i < tryUpgradeCycles; currentNetwork > 0 ? i ++ : i) {
                    if (!checkInternetConnection()) {
                        log_v("Unable to connect to the internet");
                        hasNetwork = false;
                        hasInternet = false;
                        delete network;
                        network = NULL;
                        break;
                    }
                    hasInternet = true;
                    log_v("Connected to the internet");
                    delay(checkConnectionRate);
                }
                //Try to connect to a network (if we're already connected, only check networks of higher priority)
                if (tryConnections()) {
                    if (hasNetwork) {
                        log_v("Upgrade network connection type to index %u: %u", currentNetwork, priorities[currentNetwork]);
                    } else {
                        log_v("Network connected at index %u: %u", currentNetwork, priorities[currentNetwork]);
                    }
                } else {
                    if (hasNetwork) {
                        log_v("Could not upgrade to higher priority connection");
                    } else {
                        log_v("Could not find network connection");
                        xEventGroupClearBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK | tasks::FLAG_NETWORK_IS_WIFI | tasks::FLAG_NETWORK_IS_SIM | tasks::FLAG_NETWORK_IS_BT);
                        delay(noConnectionRate);
                    }
                    continue;
                }
                if (hasNetwork) {
                    delete network;
                    network = NULL;
                }
                hasInternet = false;
                hasNetwork = true;
                switch (priorities[currentNetwork]) {
                case sys::net::NETWORK_WIFI:
                case sys::net::NETWORK_WIFI_SECONDARY:
                    network = new sys::net::wifi::Client;
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
                    log_e("Claim to have connect to an invalid network type");
                }
            }
        }

        //TODO: Make this a static member of the class, make the constructor private so that this is the only thing that can instatiate the task
        // void task(void * param) {
        //     log_v("Starting Network Task");
        //     NetworkTask t;
        //     while (!NetworkTask::deleteSelfWhenDone)
        //         t.run();
        //     vTaskDelete(NULL);
        // }

    }
}
