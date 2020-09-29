/*
 * Functions to connect to cell network (SIM card)
 */

#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE
#define LOG_LOCAL_NAME "sim"
#include "log.h"

#include "freematics.h"

#include "system/networks/sim.h"
#include "configs.h"

namespace sys {
    namespace net {
        namespace sim {

            const uint8_t SimClass::maxSockets = 10;

            SimClass::SimClass() : system(nullptr), client(), buffer{0} {}

            SimClass::SimClass(Freematics * s) : system(s), client(), buffer{0} {}

            void SimClass::setSystem(Freematics * s) {
                if (!system)
                    system = s;
            }

            bool SimClass::status() {
                //TODO: Implement some sort of status flag
                return false;
            }

            bool SimClass::isConnected() {
                LOGV("isConnected");
                return strlen(getDeviceName()) && getOperator() && getSignal() && getIP() != INADDR_NONE;
            }

            bool SimClass::scanOperators() {
                sendCommand("AT+COPS=?\r", 300000);
                //TODO: Implement choosing operator
                return false;
            }

            bool SimClass::getScanResult(uint8_t idx) {
                //TODO: Implement choosing operator
                return false;
            }

            bool SimClass::getOperator(char * buf) {
                LOGV("getOperator");
                if (!sendCommand("AT+COPS?\r", 3000, "\r\nOK\r\n")) {
                    LOGV("Command failed");
                    return false;
                }
                // LOGV("Find \"");
                char * p = strstr(buffer, ",\"");
                if (!p) {
                    LOGV("No operator name");
                    return false;
                }
                // LOGV("Find close \"");
                char * q = strchr(p+2, '\"');
                if (q)
                    *q = 0;
                else {
                    LOGV("Malformed response");
                    return false;
                }
                LOGV("Operator %s", p+2);
                if (buf)
                    strcpy(buf, p+2);
                return true;
            }

            bool SimClass::setOperator(uint8_t idx) {
                //TODO: Implement choosing operator
                return false;
            }

            int SimClass::getSignal() {
                // LOGV("getSignal");
                int i = client.getSignal();
                // LOGV("Got signal? %d", i);
                return i;
            }

            bool SimClass::checkSim() {
                return client.checkSIM();
            }

            bool SimClass::getSimProvider(char * buf) {
                LOGV("getSimProvider");
                if (!sendCommand("AT+CSPN?\r", 3000)) {
                    LOGV("Command failed");
                    return false;
                }
                // LOGV("Find \"");
                char * p = strchr(buffer, '\"');
                if (!p) {
                    LOGV("No provider name");
                    return false;
                }
                p++;
                // LOGV("Find close \"");
                char * q = strchr(p, '\"');
                if (q)
                    *q = 0;
                else {
                    LOGV("Malformed response");
                    return false;
                }
                LOGV("Provider %s", p);
                if (buf)
                    strcpy(buf, p);
                return true;
            }

            IPAddress SimClass::getIP() {
                LOGV("getIP");
                if (!sendCommand("AT+IPADDR\r", 3000))
                    return INADDR_NONE;
                char * p = strstr(buffer, "+IPADDR:");
                if (!p || *(p+9) == '0')
                    return INADDR_NONE;
                p += 9;
                char * q = strchr(p, '\r');
                if (q)
                    *q = 0;
                LOGV("Found IP %s", p);
                IPAddress ip;
                if (!ip.fromString(p))
                    return INADDR_NONE;
                LOGV("Parsed to %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
                return ip;
            }

            const char * SimClass::getIMEI() {
                return client.IMEI;
            }

            const char * SimClass::getDeviceName() {
                return client.deviceName();
            }

            bool SimClass::begin() {
                if (!system)
                    return false;
                return client.begin(system);
            }

            void SimClass::end() {
                client.end();
            }

            bool SimClass::connect() {
                if (!system)
                    return false;
                return client.setup(configs::SIM_APN);
            }

            bool SimClass::disconnect() {
                //TODO: Anything else need to be done here?
                return sendCommand("AT+NETCLOSE\r");
            }

            bool SimClass::waitForNetwork(uint32_t timeout, uint32_t interval) {
                uint32_t t = millis(), e = 0;
                while ((e = millis() - t) < timeout && (!sendCommand("AT+CPSI?\r", 1000, "Online") || strstr(buffer, "Off") || strstr(buffer, "NO SERVICE")))
                    delay(interval);
                return e < timeout;
            }

            bool SimClass::waitForConnection(uint32_t timeout, uint32_t interval) {
                uint32_t t = millis(), e = 0;
                while ((e = millis() - t) < timeout && (!getIP()))
                    delay(interval);
                return e < timeout;
            }

            bool SimClass::sendCommand(const char* cmd, unsigned int timeout, const char* expected)
            {
                if (!system)
                    return false;
                if (cmd) {
                    system->xbWrite(cmd);
                }
                buffer[0] = 0;
                byte ret = system->xbReceive(buffer, sizeof(buffer), timeout, &expected, 1);
                if (ret) {
                    return true;
                } else {
                    return false;
                }
            }

            IPAddress SimClass::queryDns(const char * host, uint32_t timeout){
                if (!isConnected())
                    return INADDR_NONE;
                char cmd[270];
                sprintf(cmd, "AT+CDNSGIP=\"%s\"", host);
                if (!sendCommand(cmd, timeout))//, "+CDNSGIP: 1"))
                    return INADDR_NONE;
                char * p = strstr(buffer, "\",\"");
                if (!p)
                    return INADDR_NONE;
                p += 3;
                char * q = strchr(p, '\"');
                if (q)
                    *q = 0;
                IPAddress ip;
                if (!ip.fromString(p))
                    return INADDR_NONE;
                return ip;
            }

            int8_t SimClass::openSocket(){
                return 0;
            }

            bool SimClass::sendOnSocket(int8_t s){
                return 0;
            }

            int32_t SimClass::receiveFromSocket(int8_t s){
                return 0;
            }

            void SimClass::closeSocket(int8_t s){

            }

            Client::Client() {}

            Client::~Client() {
                stop();
            }

            int Client::connect(IPAddress ip, uint16_t port) {
                return 0;
            }

            int Client::connect(const char * host, uint16_t port) {
                return 0;
            }

            size_t Client::write(uint8_t) {
                return 0;
            }

            size_t Client::write(const uint8_t * buf, size_t size) {
                return 0;
            }

            int Client::available() {
                return 0;
            }

            int Client::read() {
                return 0;
            }

            int Client::read(uint8_t * buf, size_t size) {
                return 0;
            }

            int Client::peek() {
                return 0;
            }

            void Client::flush() {

            }

            void Client::stop() {

            }

            uint8_t Client::connected() {
                return 0;
            }

            Client::operator bool() {
                return 0;
            }

        }
    }
}

sys::net::sim::SimClass Sim;
