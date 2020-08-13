/*
 * Generic HTTP(S) client
 */

#define LOG_LOCAL_NAME "http"
#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#include "log.h"

#include "system/http.h"

#include "freematics.h"



namespace sys {
    namespace clt {

        unsigned short urlEncode(char* dst, char* src) {
            LOGV("Unencoded: %s", src);
            const char special[] = "\"#$%&+,/:;=<>?@[]\\^~`{}| \t\n";
            const char hex[] = "0123456789ABCDEF";
            const char* s = src;
            char code[] = "%00";
            char* c = strpbrk(src, special);
            while (c) {
                LOGV("%p -> %p (%d)", (void*)s, (void*)c, c - s);
                LOGV("Text: %.*s", c - s, s);
                if (c - s)
                    strncat(dst, s, c - s);
                LOGV("Char: %c", *c);
                LOGV("Index 1: %d", (*c >> 4) & 0x0F);
                LOGV("Digit 1: %c", hex[(*c >> 4) & 0x0F]);
                LOGV("Index 2: %d", *c & 0x0F);
                LOGV("Digit 2: %c", hex[*c & 0x0F]);
                code[1] = hex[(*c >> 4) & 0x0F];
                code[2] = hex[*c & 0x0F];
                LOGV("Code: %s", code);
                strcat(dst, code);
                s = c + 1;
                c = strpbrk(s, special);
            }
            strcat(dst, s);
            LOGD("Encoded: %s", dst);
            return strlen(dst);
        }

        Parameters::Parameters(const String& p) : purpose(p) {};

        bool Parameters::add(const String& key, const String& value) {
            LOGV("add(%s, %s)", key.c_str(), value.c_str());
            if (count == MAX_COUNT)
                return false;
            params[count].key = key;
            params[count].value = value;
            params[count].key.trim();
            params[count].value.trim();
            LOGD("Adding to %s: %s=%s", purpose.c_str(), params[count].key.c_str(), params[count].value.c_str());
            count ++;
            return true;
        }

        bool Parameters::add(const String& keyvalue) {
            LOGV("add(%s)", keyvalue.c_str());
            int colon = keyvalue.indexOf(':');
            int equal = keyvalue.indexOf('=');
            int split = min(colon < 0 ? INT_MAX : colon, equal < 0 ? INT_MAX : equal);
            if (split > keyvalue.length())
                return false;
            return add(keyvalue.substring(0, split), keyvalue.substring(split + 1));
        }

        bool Parameters::set(const String& key, const String& value) {
            LOGV("set(%s, %s)", key.c_str(), value.c_str());
            short idx = find(key);
            if (idx < 0)
                return add(key, value);
            params[idx].value = value;
            params[idx].value.trim();
            LOGD("Set in %s: %s=%s", purpose.c_str(), params[idx].key.c_str(), params[idx].value.c_str());
            return true;
        }

        bool Parameters::set(const String& keyvalue) {
            LOGV("set(%s)", keyvalue.c_str());
            int colon = keyvalue.indexOf(':');
            int equal = keyvalue.indexOf('=');
            int split = min(colon < 0 ? INT_MAX : colon, equal < 0 ? INT_MAX : equal);
            if (split > keyvalue.length())
                return false;
            return set(keyvalue.substring(0, split), keyvalue.substring(split + 1));
        }

        bool Parameters::remove(const String& key) {
            LOGV("%s", key.c_str());
            short idx = find(key);
            if (idx < 0)
                return false;
            params[idx].value = "";
            return true;
        }

        String Parameters::get(const String& key) {
            LOGV("%s", key.c_str());
            short idx = find(key);
            if (idx < 0)
                return "";
            return params[idx].value;
        }

        bool Parameters::has(const String& key) {
            LOGV("%s", key.c_str());
            return find(key) >= 0;
        }

        short Parameters::find(const String& key) {
            LOGV("%s", key.c_str());
            String temp = key;
            temp.trim();
            for (short idx = 0; idx < count; idx ++) {
                if (temp == params[idx].key)
                    return idx;
            }
            return -1;
        }

        //TODO: Move all initializations here
        HTTP::HTTP() :
            reqHeaders("request headers"),
            urlParams("url parameters"),
            bodyParams("body parameters"),
            resHeaders("response headers") {}

        HTTP::~HTTP() {
            done();
        }

        bool HTTP::configure(Client * clt) {
            client = clt;
            return true;
        }

        bool HTTP::configure(Client * clt, const String &url) {
            return configure(clt) && setUrl(url);
        }

        bool HTTP::configure(Client * clt, const String& hst, unsigned short prt, const String& endp, bool s) {
            LOGD("Protocol: http%s Host: %s Port: %u Endpoint: %s", s ? "s" : "", hst.c_str(), prt, endp.c_str());
            return configure(clt) && setUrl(hst, prt, endp, s);
        }

        bool HTTP::done() {
            if (client)
                client->stop();
            return true;
        }

        bool HTTP::connected() {
            return (client && (client->available() || client->connected()));
        }

        bool HTTP::get() {
            LOGV("Adding headers");
            reqHeaders.set("User-Agent", agent);
            reqHeaders.set("Connection", reuse ? "keep-alive" : "close");
            reqHeaders.set("Host", host);
            reqHeaders.set("Content-Length", "0");
            time_t now;
            time(&now);
            tm* nowtm = gmtime(&now);
            char buf[35];
            strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
            reqHeaders.set("Date", buf);
            request.clear();
            request.http = "HTTP/1.1";
            request.method  = "GET";
            request.url = endpoint;
            for (unsigned short idx = 0; idx < urlParams.count; idx ++) {
                char src[1024], dst[1024] = "";
                strcpy(src, urlParams.params[idx].value.c_str());
                urlEncode(dst, src);
                request.url += (idx ? "&" : "?") + urlParams.params[idx].key + "=" + dst;
            }
            for (unsigned short idx = 0; idx < reqHeaders.count; idx ++)
                request.headers += reqHeaders.params[idx].key + ": " + reqHeaders.params[idx].value + "\r\n";
            request.raw = request.method + ' ' + request.url + ' ' + request.http + "\r\n" + request.headers + "\r\n";
            request.size = request.raw.length();
            LOGI("Request: (%u B)\n%s", request.size, request.raw.c_str());
            bool ret = send((unsigned char *)request.raw.c_str(), request.size);
            LOGI("Response: (%u B)\n%s", response.size, response.raw.c_str());
            LOGV("Details:\nProtocol: %s\nCode: %i %s\nMethod: %s\nUrl: %s\nHeaders:\n%s\nBody:\n%s\n",
                response.http.c_str(), response.code, response.codetext.c_str(), response.method.c_str(),
                response.url.c_str(), response.headers.c_str(), response.body.c_str());
            for (short idx = 0;  idx < resHeaders.count; idx ++)
                LOGV("Header: %s = %s", resHeaders.params[idx].key.c_str(), resHeaders.params[idx].value.c_str());
            return ret;
        }

        bool HTTP::post() {
            LOGE("HTTP POST not implemented!");
            return 0;
        }

        bool HTTP::parse(char c) {
            LOGV("parse: %i\n", c);
            static unsigned short phase = 0;
            if (!c) {
                //New response being received, prepare to receive it
                response.clear();
                resHeaders.count = 0;
                phase = 0;
                LOGV("Initializing parser (%u)", phase);
                parseBody();
                return true;
            }
            response.raw += c;
            response.size ++;
            //TODO: handle bad data in each of these
            switch (phase) {
            case 0: //Just starting
                phase++;
            case 1: //Parsing HTTP version
                if (c == ' ')
                    phase ++;
                else
                    response.http += c;
                return true;
            case 2: //Parsing response code value
                if (c == ' ') {
                    phase ++;
                    response.code = (HttpCode)response.codetext.toInt();
                    response.codetext = "";
                } else
                    response.codetext += c;
                return true;
            case 3: //Parsing response code text
                if (c == '\n' && response.codetext.endsWith("\r")) {
                    response.codetext.trim();
                    phase ++;
                } else
                    response.codetext += c;
                return true;
            case 4: //Parsing headers
                response.headers += c;
                if (response.headers.endsWith("\r\n\r\n")) {
                    response.headers.trim();
                    phase ++;
                } else if (response.headers.endsWith("\r\n")) {
                    int s = response.headers.lastIndexOf("\r\n", response.headers.length() - 3);
                    s = (s < 0 ? 0 : s + 2);
                    LOGV("Found header (%u -> %u) - %s", s, response.headers.length() - 1,
                        response.headers.substring(s, response.headers.length() - 2).c_str()
                    );
                    //TODO: This assumes one of each header, but some headers can have multiple (like Set-Cookie)
                    resHeaders.set(response.headers.substring(s, response.headers.length() - 2).c_str());
                }
                return true;
            case 5: //Parsing body
                // Serial.print(c);
                if (!parseBody(c)) {
                    response.body.trim();
                    return false;
                }
                return true;
            }
            return true;
        }

        bool HTTP::parseBody(char c) {
            LOGV("parseBody: %i\n", c);
            static unsigned short mode = 0;
            static short len = 0;
            if (!c) {
                mode = 0;
                len = 0;
                LOGV("Initializing body parser (%u, %i)", mode, len);
                return true;
            }
            while (!mode) {

                if (request.method == "HEAD" || response.code < HTTP_CODE_OK ||
                    response.code == HTTP_CODE_NO_CONTENT || response.code == HTTP_CODE_NOT_MODIFIED) {
                    //Response has no body
                    LOGV("No body to read");
                    return false;
                    break;
                }
                String hdr = resHeaders.get("Transfer-Encoding");
                if (hdr.equalsIgnoreCase("identity")) {
                    //Chunked body
                    LOGV("Chunked body");
                    mode = 1;
                    break;
                }
                hdr = resHeaders.get("Content-Length");
                if (hdr.length()) {
                    //Body length provided
                    LOGV("Fixed length body");
                    mode = 4;
                    len = hdr.toInt();
                    break;
                }
                hdr = resHeaders.get("Content-Type");
                if (hdr.substring(0, 10).equalsIgnoreCase("multipart/")) {
                    //Multipart (like form data), Note - not implemented
                    LOGV("Multipart body");
                    mode = 5;
                    break;
                }
                LOGD("Unknown body");
                mode = 6;
            }
            switch (mode) {
            case 1: //Chunked body - reading size
                if (c >= '0' && c <= '9')
                    len = len * 10 + (c - '0');
                if (c == '\r')
                    mode = 2;
                return true;
            case 2:
                if (c == '\n')
                    mode = 3;
                else
                    mode = 1;
                return true;
            case 3: //Chunked body - reading content
                response.body += c;
                if (response.body.endsWith("\r\n")) {
                    response.body.trim();
                    if (len)
                        return true;
                    return false;
                }
                return true;
            case 4: //Fixed length body
                response.body += c;
                len --;
                return (len > 0);
            case 5: //Multipart data
                LOGW("Multipart response parsing not implemented");
                return false;
            case 6: //Unknown length, read until disconnected
                response.body += c;
                return true;
            }
            return false;
        }

        bool HTTP::setUrl(const String& url) {
            LOGV("Parsing url %s", url.c_str());

            short sep, colon, slash, qmark, amp, equal;
            bool https;
            String host, endpoint;
            unsigned short port;

            //Find protocol
            https = url.startsWith("https");

            //Separate host:port and endpoint
            sep = url.indexOf("://");
            sep = (sep < 0 ? 0 : sep + 3);
            slash = url.indexOf("/", sep);

            //Parse host
            if (slash < 0)
                host = url.substring(sep);
            else
                host = url.substring(sep, slash);

            //Parse port
            colon = host.indexOf(":");
            if (colon < 0)
                port = (https ? 443 : 80);
            else {
                port = host.substring(colon + 1).toInt();
                host.remove(colon);
            }

            //Parse endpoint
            if (slash < 0) {
                endpoint = "/";
                qmark = -1;
            } else {
                qmark = url.indexOf("?");
                if (qmark < 0)
                    endpoint = url.substring(slash);
                else
                    endpoint = url.substring(slash, qmark);
            }

            //Set components of url
            bool ret = setUrl(host, port, endpoint, https);

            //Parse url parameters
            while (qmark >= 0) {
                qmark ++;
                equal = url.indexOf("=", qmark);
                amp = url.indexOf("&", qmark);
                LOGV("%d %d %d", qmark, equal, amp);
                urlParams.add(url.substring(qmark, equal), (amp < 0 ? url.substring(equal + 1) : url.substring(equal + 1, amp)));
                qmark = amp;
            }

            return ret;
        }

        bool HTTP::setUrl(const String& hst, unsigned short prt, const String& endp, bool s) {
            https = s;
            host = hst;
            port = prt;
            endpoint = endp;
            return true;
        }

        void HTTP::setReuse(const bool ru) {
            reuse = ru;
        }

        void HTTP::setUserAgent(const String& ua) {
            agent = ua;
        }

        void HTTP::setPostFormat(const PostParameterFormat form) {
            postFormat = form;
        }

        void HTTP::setTimeout(const int to) {
            timeout = to;
            //TODO: what else needs to be done here with this number
        }

        bool HTTP::send(unsigned char* req, unsigned int size) {
            static int failures = 0;
            if (!connected()) {
                LOGD("Client? %c, Connected? %d  Available? %d",
                    (bool)client ? 'Y' : 'N', client->connected(), client->available());
                LOGV("Establishing connection");
                if (!connect() || !connected()) {
                    LOGI("Failed to connect");
                    return false;
                }
            } else if (client->available()) {
                LOGD("Clearing received data (%u B)", client->available());
                client->flush();
            }
            if (request.raw.length() == 0) {
                LOGI("No request to send");
                return false;
            }
            unsigned int s = client->write(req, size);
            LOGD("Sent %u, expected %u", s, size);
            if (s < size) {
                LOGW("Failed to send data");
                if (++failures == 3) {
                    LOGW("Three consecutive failures, disconnecting");
                    disconnect();
                }
                return false;
            }
            LOGV("Request sent");
            if (!receive()) {
                LOGW("Failed to process response");
                if (++failures == 3) {
                    LOGW("Three consecutive failures, disconnecting");
                    disconnect();
                }
                return false;
            }
            LOGV("Response processed");
            failures = 0;
            return true;
        }

        unsigned int HTTP::receive() {
            LOGV("Receiving data");
            unsigned int size = 0;
            if (!connected()) {
                LOGI("No connection to receive from");
                return 0;
            }
            if (!parse()) {
                LOGI("Not ready to parse");
                return 0;
            }
            unsigned long t = millis();
            while (true) {
                if (!connected()) {
                    LOGW("Lost connection while receiving data");
                    return 0;
                }
                int avail = client->available();
                if (!avail) {
                    if (millis() - t > timeout) {
                        LOGI("Timed out while receiving data");
                        return 0;
                    }
                    delay(timeout / 10);
                    continue;
                }
                size ++;
                if (!parse(client->read())) {
                    LOGV("HTTP parsing complete (%u B)", size);
                    return size;
                }
                t = millis();
            }
        }

        bool HTTP::connect() {
            if (connected()) {
                LOGV("Already connected");
                client->flush();
                return false;
            }
            if (!client) {
                LOGE("No valid client to connect");
                return false;
            }
            //TODO: generic client has no timeout, but WiFiClient does...  Ah, Stream does, too!
            if (!client->connect(host.c_str(), port)) {
                LOGE("Failed to connect to host");
                return false;
            }
            LOGI("Connected to %s", host.c_str());
            return connected();
        }

        void HTTP::disconnect() {
            if (!connected()) {
                LOGD("Already disconnected");
                return;
            }
            if (reuse)
                LOGI("Reuse is set, forcibly disconnecting");
            client->flush();
            client->stop();
            LOGI("Connection terminated");
        }

    }
}
