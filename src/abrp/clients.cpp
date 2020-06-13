/*
 * Generic HTTP(S) client
 */

#include "abrp/clients.h"

#include <FreematicsPlus.h>

namespace abrp {

    namespace clients {

        String urlEncode(const String& str) {
            String ret = "";
            const unsigned char special[] = "\"#$%&+,/:;=<>?@[]\\^~`{}| \t\n";
            const unsigned char hex[] = "0123456789ABCDEF";
        }

        Parameters::Parameters(const String& p) : purpose(p) {};

        bool Parameters::add(const String& key, const String& value) {
            if (count == 15)
                return false;
            log_v("Adding to %s: %s=%s", purpose.c_str(), key.c_str(), value.c_str());
            params[count].key = key;
            params[count].value = value;
            count ++;
            return true;
        }

        bool Parameters::set(const String& key, const String& value) {
            unsigned short idx = find(key);
            if (idx < 0)
                return add(key, value);
            params[idx].value = value;
            return true;
        }

        bool Parameters::remove(const String& key) {
            unsigned short idx = find(key);
            if (idx < 0)
                return false;
            params[idx].value = "";
            return true;
        }

        String Parameters::get(const String& key) {
            unsigned short idx = find(key);
            if (idx < 0)
                return "";
            return params[idx].value;
        }

        bool Parameters::has(const String& key) {
            return find(key) >= 0;
        }

        unsigned short Parameters::find(const String& key) {
            for (unsigned short idx = 0; idx < count; idx ++) {
                if (key == params[idx].key)
                    return true;
            }
            return false;
        }

        HTTP::HTTP() : reqHeaders("request headers"), urlParams("url parameters"),
            bodyParams("body parameters"), resHeaders("response headers") {}

        HTTP::~HTTP() {
            done();
        }

        bool HTTP::configure(Client &clt, const String &url) {
            log_v("Parsing url %s", url.c_str());

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

            //Configure client
            bool ret = configure(clt, host, port, endpoint, https);

            //Parse url parameters
            while (qmark >= 0) {
                qmark ++;
                equal = url.indexOf("=", qmark);
                amp = url.indexOf("&", qmark);
                // log_v("%d %d %d", qmark, equal, amp);
                urlParams.add(url.substring(qmark, equal), (amp < 0 ? url.substring(equal + 1) : url.substring(equal + 1, amp)));
                qmark = amp;
            }

            return ret;
        }

        bool HTTP::configure(Client &clt, const String& hst, unsigned short prt, const String& endp, bool s) {
            log_v("Protocol: http%s Host: %s Port: %u Endpoint: %s", s ? "s" : "", hst.c_str(), prt, endp.c_str());
            client = &clt;
            https = s;
            host = hst;
            port = prt;
            endpoint = endp;
            return true;
        }

        // bool HTTP::connect() {
        //     return true;
        // }

        // bool HTTP::disconnect() {
        //     return true;
        // }

        bool HTTP::done() {
            if (client)
                client->stop();
            return true;
        }

        bool HTTP::connected() {
            return (client && (client->available() || client->connected()));
        }

        bool HTTP::get() {
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
            request.http = "HTTP/1.1";
            request.method  = "GET";
            request.url = endpoint;
            for (unsigned short idx = 0; idx < urlParams.count; idx ++)
                request.url += (idx ? "&" : "?") + urlParams.params[idx].key + "=" + urlEcnode(urlParams.params[idx].value);
            request.headers = "";
            for (unsigned short idx = 0; idx < reqHeaders.count; idx ++)
                request.url += reqHeaders.params[idx].key + ": " + reqHeaders.params[idx].value + "\n";
            request.body = "";
            request.raw = request.method + " " + request.url + " " + request.http + "\n" + request.headers;
        }

        bool HTTP::post() {
            return 0;
        }

        void HTTP::setReuse(const bool ru) {
            reuse = ru;
            //TODO: set header here?  Or wait till sending request
        }

        void HTTP::setUserAgent(const String& ua) {
            agent = ua;
            //TODO: set header here?  Or wait till sending request
        }

        void HTTP::setPostFormat(const PostParameterFormat form) {
            postFormat = form;
        }

        void HTTP::setTimeout(const int to) {
            timeout = to;
            //TODO: what else needs to be done here with this number
        }

        void HTTP::connectStream() {

        }

        void HTTP::disconnectStream() {

        }

        bool HTTP::send() {

        }

        void HTTP::writeStream() {

        }

        void HTTP::readStream() {

        }


    }

}
