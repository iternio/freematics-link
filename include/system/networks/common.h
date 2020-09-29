/*
 * Header wrapping generic classes common to all interfaces
 */

#pragma once

#include <Client.h>

namespace sys {
    namespace net {

        class Interface {
            virtual bool init() = 0;
            virtual bool config() = 0;
            virtual bool begin() = 0;
            virtual bool end() = 0;
            virtual bool connect() = 0;
            virtual bool disconnect() = 0;
            virtual IPAddress getIP() = 0;
            virtual int status() = 0;
            virtual bool connected() = 0;
            virtual bool waitForConnection() = 0;
        };

        class Client {

        };

    }
}
