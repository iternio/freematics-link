/*
 * Header wrapping Freematics WiFi network
 */

#pragma once

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

namespace sys {
    namespace net {
        namespace wifi {

            class Client : public ::WiFiClient {

            };

            class ClientSecure : public ::WiFiClientSecure {

            };

        }
    }
}
