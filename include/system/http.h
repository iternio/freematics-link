/*
 * Generic HTTP(S) client
 */

#pragma once

// #include <string.h>
#include <client.h>

namespace sys {
    namespace clt {

        enum HttpCode {
            HTTP_CODE_NONE = 0,
            HTTP_CODE_CONTINUE = 100,
            HTTP_CODE_SWITCHING_PROTOCOLS = 101,
            HTTP_CODE_PROCESSING = 102,
            HTTP_CODE_OK = 200,
            HTTP_CODE_CREATED = 201,
            HTTP_CODE_ACCEPTED = 202,
            HTTP_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
            HTTP_CODE_NO_CONTENT = 204,
            HTTP_CODE_RESET_CONTENT = 205,
            HTTP_CODE_PARTIAL_CONTENT = 206,
            HTTP_CODE_MULTI_STATUS = 207,
            HTTP_CODE_ALREADY_REPORTED = 208,
            HTTP_CODE_IM_USED = 226,
            HTTP_CODE_MULTIPLE_CHOICES = 300,
            HTTP_CODE_MOVED_PERMANENTLY = 301,
            HTTP_CODE_FOUND = 302,
            HTTP_CODE_SEE_OTHER = 303,
            HTTP_CODE_NOT_MODIFIED = 304,
            HTTP_CODE_USE_PROXY = 305,
            HTTP_CODE_TEMPORARY_REDIRECT = 307,
            HTTP_CODE_PERMANENT_REDIRECT = 308,
            HTTP_CODE_BAD_REQUEST = 400,
            HTTP_CODE_UNAUTHORIZED = 401,
            HTTP_CODE_PAYMENT_REQUIRED = 402,
            HTTP_CODE_FORBIDDEN = 403,
            HTTP_CODE_NOT_FOUND = 404,
            HTTP_CODE_METHOD_NOT_ALLOWED = 405,
            HTTP_CODE_NOT_ACCEPTABLE = 406,
            HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
            HTTP_CODE_REQUEST_TIMEOUT = 408,
            HTTP_CODE_CONFLICT = 409,
            HTTP_CODE_GONE = 410,
            HTTP_CODE_LENGTH_REQUIRED = 411,
            HTTP_CODE_PRECONDITION_FAILED = 412,
            HTTP_CODE_PAYLOAD_TOO_LARGE = 413,
            HTTP_CODE_URI_TOO_LONG = 414,
            HTTP_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
            HTTP_CODE_RANGE_NOT_SATISFIABLE = 416,
            HTTP_CODE_EXPECTATION_FAILED = 417,
            HTTP_CODE_MISDIRECTED_REQUEST = 421,
            HTTP_CODE_UNPROCESSABLE_ENTITY = 422,
            HTTP_CODE_LOCKED = 423,
            HTTP_CODE_FAILED_DEPENDENCY = 424,
            HTTP_CODE_UPGRADE_REQUIRED = 426,
            HTTP_CODE_PRECONDITION_REQUIRED = 428,
            HTTP_CODE_TOO_MANY_REQUESTS = 429,
            HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
            HTTP_CODE_INTERNAL_SERVER_ERROR = 500,
            HTTP_CODE_NOT_IMPLEMENTED = 501,
            HTTP_CODE_BAD_GATEWAY = 502,
            HTTP_CODE_SERVICE_UNAVAILABLE = 503,
            HTTP_CODE_GATEWAY_TIMEOUT = 504,
            HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
            HTTP_CODE_VARIANT_ALSO_NEGOTIATES = 506,
            HTTP_CODE_INSUFFICIENT_STORAGE = 507,
            HTTP_CODE_LOOP_DETECTED = 508,
            HTTP_CODE_NOT_EXTENDED = 510,
            HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511
        };

        enum HttpMethod {
            HTTP_METHOD_GET,
            HTTP_METHOD_HEAD,
            HTTP_METHOD_POST,
            HTTP_METHOD_PUT,
            HTTP_METHOD_DELETE,
            HTTP_METHOD_CONNECT,
            HTTP_METHOD_OPTIONS,
            HTTP_METHOD_TRACE,
            HTTP_METHOD_PATCH
        };

        enum PostParameterFormat {
            POST_FORMAT_FORM_DATA,      //Format post parameters as form-data w/ appropriate boundaries
            POST_FORMAT_URL_ENCODED,    //Format post parameters as URL encoded query string in the body
            POST_FORMAT_JSON,           //Format post parameters as a JSON object in the body
            POST_FORMAT_RAW,            //Don't format post parameters, just put the value of the first parameter in the body
            POST_FORMAT_NONE            //Don't put anything in the body, ignore post parameters all together
        };

        struct Parameter {
            String key = "";
            String value = "";
        };

        unsigned short urlEncode(char* dst, char* src);

        class Parameters {
        public:
            Parameters(const String& p);
            bool add(const String& key, const String& value);
            bool add(const String& keyvalue);
            bool set(const String& key, const String& value);
            bool set(const String& keyvalue);
            bool remove(const String& key);
            String get(const String& key);
            bool has(const String& key);
        private:
            short find(const String& key);
            Parameter params[20];   //TODO: Is 15 enough?
            short count = 0;
            const String purpose;
            //TODO: Any sort of management of headers that can't be duplicated?

            static const short MAX_COUNT = 20;
            friend class HTTP;
        };

        struct Request {
            String http;
            String method;
            String url;
            String headers;
            String body;
            String raw;
            unsigned int size;
            void virtual clear() {
                http = "";
                method = "";
                url = "";
                headers = "";
                body = "";
                raw = "";
                size = 0;
            }
        };

        struct Response : public Request {
            HttpCode code;
            String codetext;
            void virtual clear() {
                Request::clear();
                code = HTTP_CODE_NONE;
                codetext = "";
            }
        };

        //TODO: Lots of String here, is this a good idea?
        class HTTP {
        public:
            HTTP();
            virtual ~HTTP();

            virtual bool configure(Client * clt);
            virtual bool configure(Client * clt, const String& url);
            virtual bool configure(Client * clt, const String& host, unsigned short port, const String& endpoint, bool https);
            virtual bool done();

            virtual bool connected();

            virtual bool get();
            virtual bool post();

            virtual bool setUrl(const String& url);
            virtual bool setUrl(const String& hst, unsigned short prt, const String& endp, bool s);
            virtual void setReuse(const bool ru);
            virtual void setUserAgent(const String& ua);
            virtual void setPostFormat(const PostParameterFormat form);
            virtual void setTimeout(const int to);

            Parameters reqHeaders;
            Parameters urlParams;
            Parameters bodyParams;
            Parameters resHeaders;

            Request request;
            Response response;

        protected:
            virtual bool send(unsigned char* req, unsigned int size);
            virtual unsigned int receive();
            virtual bool parse(char c = 0);
            virtual bool parseBody(char c = 0);

            virtual bool connect();
            virtual void disconnect();

            //TODO: Clean up the below and move values to ctro init list
            Client* client = nullptr;

            int timeout = 5000;
            bool reuse = true;
            String agent = "freematics-link/0.1 (Freematics One+ Model B)";
            PostParameterFormat postFormat = POST_FORMAT_URL_ENCODED;

            bool https = false;
            String host = "";
            unsigned short port = 80;
            String endpoint = "/";

        };

        class HTTPS : public HTTP {
        };

    }
}
