/*
 * Header wrapping Freematics cell network (SIM card)
 */

#pragma once

// #include <string.h>
#include "freematics.h"
#include <Client.h>

#include "common.h"

//This is a stopgap implementation. It doesn't use the system's network interface
//api (netif), and so the system won't see the cell chip as an internet connection.
//This means api calls, such as the LwIP SNTP or HTTP implementation, can't use
//the cell network to reach the internet.  The better way to do this is to implement
//the sim connection as a netif driver that the system can see.  This does require
//reimplementing the wifi connection in the same way, but it would mean not having
//to implement custom NTP or HTTP (or likely even HTTPS)

namespace sys {
    namespace net {
        namespace sim {

            class SimInterface : public Interface {

            };

            class SimClass {
            public:
                SimClass();
                SimClass(Freematics * s);
                void setSystem(Freematics * s);
                bool status();
                bool isConnected();
                bool scanOperators();
                bool getScanResult(uint8_t idx);
                bool getOperator(char * buf = nullptr);
                bool setOperator(uint8_t idx);
                int getSignal();
                bool checkSim();
                bool getSimProvider(char * buf = nullptr);
                IPAddress getIP();
                const char * getIMEI();
                const char * getDeviceName();
                bool begin();
                void end();
                bool connect();
                bool disconnect();
                bool waitForNetwork(uint32_t timeout = 60000, uint32_t interval = 500);
                bool waitForConnection(uint32_t timeout = 60000, uint32_t interval = 500);
                IPAddress queryDns(const char * host, uint32_t timeout = 60000);
                int8_t openSocket();
                bool sendOnSocket(int8_t s);
                int32_t receiveFromSocket(int8_t s);
                void closeSocket(int8_t s);
            protected:
                bool sendCommand(const char* cmd, unsigned int timeout = 1000, const char* expected = "\r\nOK\r\n");
                uint8_t getNextSocket();
                Freematics * system;
                ClientSIM7600 client;
                char buffer[384];
                static const uint8_t maxSockets;
                uint16_t sockets;
            };

            class Client : public ::Client {
            public:
                Client();
                virtual ~Client();
                virtual int connect(IPAddress ip, uint16_t port);
                virtual int connect(const char *host, uint16_t port);
                virtual size_t write(uint8_t c);
                virtual size_t write(const uint8_t *buf, size_t size);
                virtual int available();
                virtual int read();
                virtual int read(uint8_t *buf, size_t size);
                virtual int peek();
                virtual void flush();
                virtual void stop();
                virtual uint8_t connected();
                virtual operator bool();
            };

        }
    }
}

extern sys::net::sim::SimClass Sim;
