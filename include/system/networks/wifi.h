/*
 * Header wrapping Freematics WiFi network
 */

#pragma once

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

//TODO: This right now is set up to just use the existing Arduino WiFi
//infrastructure.  It would seem that that's the best and most expedient
//way to use WiFi, since clearly a bunch of dev work has already been put
//into it.  There are a couple caveats and potential drawbacks to this,
//tho:
// -The WiFi object & related components hide the low level ESP & NETIF
//  function calls.  Not bad per se, but this implementation generally
//  assumes you know a priori how your app will connect to the internet,
//  and that it will always use the same interface.  In our case, that's
//  not true, we want to switch between wifi, cell, & maybe bluetooth as
//  needed.
// -There's some bug somewhere that means trying to scan for networks while
//  still connecting to a network crashes the device rather than just
//  returning failure.
// -The WiFi class still uses the old style ESP TCP/IP stack rather than
//  the newer ESP NETIF API.  That's fine for WiFi, but it means that it's
//  not possible to write custom netif's for other modes (like cell or bt)
//Therefore, future implementations should skip using the WiFi object
//provided by the Arduino framework and implement a custom wrapper for the
//newer API.  This may also reduce application size, because the WiFi class
//does a lot of things we likely don't need.

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
