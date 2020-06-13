/* This library exists solely because Freematics defines its own HTTPClient class
 * which is far less functional than the built in Arduino HTTPClient class.  This
 * is a wrapper around the native class to avoid the name conflict with the
 * Freematics system library
 */

#ifndef INC_HTTPCLIENTWRAPPER_H
#define INC_HTTPCLIENTWRAPPER_H

#include <Arduino.h>
#include <WiFiClient.h>

namespace MyHTTP {

class MyClient {
public:
    bool begin(String url, const char* CAcert);
    void addHeader(const String& name, const String& value, bool first = false, bool replace = true);
    int get();
    int post(uint8_t * payload, size_t size);
    int post(String payload);
    WiFiClient& getStream(void);
};

}

#endif
