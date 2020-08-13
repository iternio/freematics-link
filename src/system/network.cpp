/*
 * Implementation for clients using network interfaces of Freematics in a standard interface
 */

#define LOG_LOCAL_NAME "net"
#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#include "log.h"

#include "system/network.h"
#include "configs.h"

#include "freematics.h"



namespace sys {
    namespace net {

        //TODO: Eventually, we may want to do something besides HTTP.  For example, Bluetooth might better use TCP or UDP to stream data
        // clt::HTTP* getClient() {
        //     const NetworkPriorities priorities[] = {NETWORK_WIFI, NETWORK_SIM};
        //     static ::WiFiClient wifi;
        //     static clt::HTTP http;
        //     //TODO: This might be better off as a class to better perform instance management

        //     for (int i = 0; i < sizeof(priorities) / sizeof(NetworkPriorities); i ++) {
        //         unsigned short tries = 0;
        //         switch(priorities[i]) {
        //         case NETWORK_WIFI:
        //             log_v("Attempting to connect to WiFi network %s", configs::SSID);
        //             while (!WiFi.isConnected() && tries++ < 5) {
        //                 WiFi.begin(configs::SSID, configs::PASSWORD);
        //                 WiFi.waitForConnectResult();
        //                 if (!WiFi.isConnected() && tries < 5) {
        //                     log_v("Failed to connect");
        //                     delay(5000);
        //                     log_v("Retrying");
        //                 }
        //             }
        //             if (!WiFi.isConnected()) {
        //                 log_v("Failed to connect to WiFi");
        //                 break;
        //             }
        //             log_v("Connected to %s", configs::SSID);
        //             http.configure(wifi);   //TODO: Is this ok to just continually reconfigure?
        //             return &http;
        //         case NETWORK_SIM:
        //             log_e("Cell data client not implemented");
        //             break;
        //         case NETWORK_BT:
        //             log_e("Bluetooth client not implemented");
        //             break;
        //         }
        //     }
        //     return nullptr;
        // }

    }
}
