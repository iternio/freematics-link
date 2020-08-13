/*
 * Task to initialize system
 */

#define LOG_LOCAL_LEVEL ARDUHAL_LOG_LEVEL_INFO
#define LOG_LOCAL_NAME "init t"
#include "log.h"

#include "freematics.h"

#ifdef BOARD_HAS_PSRAM
#ifdef CONFIG_USING_ESPIDF
#include <esp32/himem.h>
#else
#include <esp_himem.h>
#endif
#endif
#include <lwip/apps/sntp.h>

#include "tasks/common.h"
#include "tasks/init.h"
#include "configs.h"
#include "util.h"



namespace tasks {
    namespace init {

        void task(void * param) {
            ::FreematicsESP32 * system = (::FreematicsESP32 *)param;

            //Perform initial system set up
#if !CONFIG_AUTOSTART_ARDUINO
            LOGI("Initializing Arduino system");
            util::ptl();
            initArduino();
            util::ptl();
#endif
            LOGI("Initializing Freematics system");
            util::ptl();
            if (!system->begin()) {
                LOGE("Freematics system failed to initialize");
                delay(10000);
                assert(false);
            }
            util::ptl();

            esp_chip_info_t cinfo;
            esp_chip_info(&cinfo);
            LOGI("Chip: %s", cinfo.model == CHIP_ESP32 ? "ESP32" : "Unknown");
            LOGI("Cores: %u", cinfo.cores);
            LOGI("Rev: %u", cinfo.revision);
            LOGI("Features: EMB %c, WiFi %c, BLE %c, BT %c",
                cinfo.features & CHIP_FEATURE_EMB_FLASH ? 'Y' : 'N',
                cinfo.features & CHIP_FEATURE_WIFI_BGN ? 'Y' : 'N',
                cinfo.features & CHIP_FEATURE_BLE ? 'Y' : 'N',
                cinfo.features & CHIP_FEATURE_BT ? 'Y' : 'N');
            LOGI("CPU: %u MHz", ESP.getCpuFreqMHz());
            //TODO: I'm pretty sure this board is supposed to have an RTC, figure this out
            int rtc = rtc_clk_slow_freq_get();
            if (rtc)
                LOGI("RTC: %i", rtc);
            LOGI("Firmware: R%i", system->version);
            LOGI("Flash: %u B", ESP.getFlashChipSize());
            LOGI("Flash (SPI): %u B", spi_flash_get_chip_size());
            LOGI("IRAM: %u B", ESP.getHeapSize());
#ifdef BOARD_HAS_PSRAM
            LOGI("PSRAM: %u B", ESP.getPsramSize() + esp_himem_get_phys_size());
            LOGI("SPIRAM: %u B", ESP.getPsramSize());
            LOGI("HiMem Phys Size: %u B", esp_himem_get_phys_size());
            LOGI("HiMem Reserved: %u B", esp_himem_reserved_area_size());
#endif

            //Notify main task initial system set up complete, ready to find internet connection
            xTaskNotifyGive(taskHandles.taskMain);

            //Wait for internet connection
            while (!(tasks::FLAG_HAS_NETWORK & xEventGroupWaitBits(taskHandles.flags, tasks::FLAG_HAS_NETWORK, pdFALSE, pdTRUE, 100)));

            //Set system time via NTP
            //TODO: Sometimes this takes forever, figure out why, maybe stop & restart?
            time_t now;
            tm* nowtm;
            char buf[35];
            time(&now);
            nowtm = gmtime(&now);
            strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
            LOGD("Current system time: %s", buf);
            LOGI("Getting time via SNTP");
            sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_setservername(0, (char*)"pool.ntp.org");
            sntp_init();
            delay(500);
            while (time(nullptr) < (2020 - 1970) * 365 * 24 * 3600)
                delay(1000);
            time(&now);
            nowtm = gmtime(&now);
            strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
            LOGI("System time updated to: %s", buf);

            //Notify main task that time is set and it can proceed
            xTaskNotifyGive(taskHandles.taskMain);

            //Initialization done, delete task
            taskHandles.taskInit = NULL;
            vTaskDelete(NULL);
            LOGE("Task failed to delete (shouldn't ever get here)");
        }

    }
}
