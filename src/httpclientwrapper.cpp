/* This library exists solely because Freematics defines its own HTTPClient class
 * which is far less functional than the built in Arduino HTTPClient class.  This
 * is a wrapper around the native class to avoid the name conflict with the
 * Freematics system library
 */

#include "httpclientwrapper.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

namespace MyHTTP {

::HTTPClient client;

bool MyClient::begin(String url, const char* CAcert) {
    return client.begin(url, CAcert);
}

void MyClient::addHeader(const String& name, const String& value, bool first, bool replace) {
    client.addHeader(name, value, first, replace);
}

int MyClient::get() {
    return client.GET();
}

int MyClient::post(uint8_t * payload, size_t size) {
    return client.POST(payload, size);
}

int MyClient::post(String payload) {
    return client.POST(payload);
}

WiFiClient& MyClient::getStream() {
    return client.getStream();
}


}
