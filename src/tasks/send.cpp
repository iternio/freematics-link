/*
 * Task to send JSON telemetry strings to ABRP
 */

#include "freematics.h"
#include <Client.h>

#include "tasks/common.h"
#include "tasks/send.h"
#include "system/network.h"
#include "abrp/params.h"
#include "configs.h"

namespace tasks {
    namespace send {

        const char SenderTask::name[] = "send";
        const uint32_t SenderTask::memory = 8192;
        const uint8_t SenderTask::priority = 10;

        // const TickType_t SenderTask::rate = pdMS_TO_TICKS(10000);

        SenderTask::SenderTask(void * p) :
            Task(p),
            state(STATE_INIT),
            queue(taskHandles.queueTelem2Send),
            flags(taskHandles.flags),
            network(NULL),
            type(FLAG_NONE),
            failures(0),
            http(),
            data("") {}

        void SenderTask::run() {
            while (true) {
                switch (state) {
                case STATE_INIT:
                    if (!doInit()) {
                        log_v("Sender task failed to init");
                        delay(1000);
                        break;
                    }
                    state = STATE_NO_NETWORK;
                    break;
                case STATE_NO_NETWORK:
                    if (!getNetworkConnection()) {
                        log_v("Could not find network connection");
                        if (failures ++ >= 2)
                            state = STATE_WAITING_FOR_DATA;
                        delay(1000);
                        break;
                    }
                    state = STATE_WAITING_FOR_DATA;
                    break;
                case STATE_WAITING_FOR_DATA:
                    if (checkForNetworkConnection()) {
                        log_v("Have network connection");
                        if (getTelemInBuffer() || waitForTelemInQueue()) {
                            log_v("Have data to send");
                            state = STATE_DATA_TO_SEND;
                        }
                    } else {
                        if (waitForTelemInQueue()) {
                            log_v("Don't have network connection, save data to buffer first");
                            state = STATE_DATA_TO_BUFFER;
                        } else {
                            log_v("Don't have network connection");
                            state = STATE_NO_NETWORK;
                        }
                    }
                    failures = 0;
                    break;
                case STATE_DATA_TO_BUFFER:
                    if (!saveTelemToBuffer()) {
                        log_v("Failed to save data to buffer!");
                        if (failures ++ >= 4)
                            state = STATE_NO_NETWORK;
                        delay(1000);
                        break;
                    }
                    state = STATE_WAITING_FOR_DATA;
                    break;
                case STATE_DATA_TO_SEND:
                    if (!sendTelemToServer()) {
                        log_v("Failed to send data to server!");
                        if (failures ++ >= 4)
                            state = STATE_NO_NETWORK;
                        delay(1000);
                        break;
                    }
                    state = STATE_WAITING_FOR_DATA;
                    break;
                default:
                    log_v("Invalid state!!!");
                    delay(1000);
                }
            }
        }

        bool SenderTask::doInit() {
            log_v("Initializing HTTP client parameters");
            char url[50], authkey[50];
            strcpy(url, abrp::params::PROTOCOL);
            strcat(url, abrp::params::HOST);
            strcat(url, abrp::params::SEND_ENDPOINT);
            strcpy(authkey, abrp::params::HEADER_AUTH_TEXT);
            strcat(authkey, configs::APIKEY);
            http.setUrl(url);
            http.urlParams.set(abrp::params::VAR_TOKEN, configs::TOKEN);
            http.reqHeaders.set(abrp::params::HEADER_AUTH, authkey);
            return true;
        }

        bool SenderTask::getNetworkConnection() {
            log_v("Getting network connection");
            if (network)
                log_e("Network should already be deleted to avoid memory leaks!");
            EventBits_t bits = xEventGroupGetBits(flags);
            if (!(bits & FLAG_HAS_NETWORK))
                return false;
            if (bits & FLAG_NETWORK_IS_WIFI) {
                log_v("WiFi connection available");
                network = new sys::net::wifi::Client;
                if (http.configure(network)) {
                    type = FLAG_NETWORK_IS_WIFI;
                    return true;
                }
            } else if (bits & FLAG_NETWORK_IS_SIM || bits & FLAG_NETWORK_IS_BT) {
                log_v("Non-wifi network clients not yet implemented");
                return false;
            }
            log_v("Inconsistent network flags...");
            return false;
        }

        bool SenderTask::checkForNetworkConnection() {
            log_v("Checking for network");
            EventBits_t bits = xEventGroupGetBits(flags);
            if (!(bits & FLAG_HAS_NETWORK) || !network->connected() || !network->connect(abrp::params::HOST, 80)) {
                log_v("No connection available");
                http.configure(nullptr);
                delete network;
                network = nullptr;
                type = FLAG_NONE;
                return false;
            }
            return true;
        }

        bool SenderTask::getTelemInBuffer() {
            //TODO: Implement some sort of buffer for when there's no network connection
            return false;
        }

        bool SenderTask::waitForTelemInQueue() {
            log_v("Waiting for item in queue");
            return xQueueReceive(queue, data, pdMS_TO_TICKS(1000));
        }

        bool SenderTask::sendTelemToServer() {
            log_v("Sending data to server via GET: %s", data);
            http.urlParams.set(abrp::params::VAR_TELEM, data);
            // if (!http.get()) {
            //     log_v("Sending data failed");
            //     return false;
            // }
            return true;
        }

        bool SenderTask::saveTelemToBuffer() {
            //TODO: Implement buffer
            return false;
        }


    }
}
