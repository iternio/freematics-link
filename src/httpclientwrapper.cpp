/* This library exists solely because Freematics defines its own HTTPClient class
 * which is far less functional than the built in Arduino HTTPClient class.  This
 * is a wrapper around the native class to avoid the name conflict with the
 * Freematics system library
 */

#include "httpclientwrapper.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

namespace HTTP {

::HTTPClient client;

bool Client::begin(String url, const char* CAcert) {
    return client.begin(url, CAcert);
}

void Client::addHeader(const String& name, const String& value, bool first, bool replace) {
    client.addHeader(name, value, first, replace);
}

int Client::get() {
    return client.GET();
}

int Client::post(uint8_t * payload, size_t size) {
    return client.POST(payload, size);
}

int Client::post(String payload) {
    return client.POST(payload);
}

WiFiClient& Client::getStream() {
    return client.getStream();
}


}
