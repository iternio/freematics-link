/*
 * Task to initialize system
 */

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

namespace tasks {
    namespace init {

        void task(void * param) {
            ::FreematicsESP32 * system = (::FreematicsESP32 *)param;

            //Perform initial system set up
#if !CONFIG_AUTOSTART_ARDUINO
            log_v("Initializing Arduino system");
            initArduino();
#endif
            log_v("Initializing Freematics system");
            if (!system->begin()) {
                log_e("Freematics system failed to initialize");
                delay(10000);
                assert(false);
            }

            esp_chip_info_t cinfo;
            esp_chip_info(&cinfo);
            log_i("Chip: %s", cinfo.model == CHIP_ESP32 ? "ESP32" : "Unknown");
            log_i("Cores: %u", cinfo.cores);
            log_i("Rev: %u", cinfo.revision);
            log_i("Features: EMB %c, WiFi %c, BLE %c, BT %c",
                cinfo.features & CHIP_FEATURE_EMB_FLASH ? 'Y' : 'N',
                cinfo.features & CHIP_FEATURE_WIFI_BGN ? 'Y' : 'N',
                cinfo.features & CHIP_FEATURE_BLE ? 'Y' : 'N',
                cinfo.features & CHIP_FEATURE_BT ? 'Y' : 'N');
            log_i("CPU: %u MHz", ESP.getCpuFreqMHz());
            //TODO: I'm pretty sure this board is supposed to have an RTC, figure this out
            int rtc = rtc_clk_slow_freq_get();
            if (rtc)
                log_i("RTC: %i", rtc);
            log_i("Firmware: R%i", system->version);
            log_i("Flash: %u B", ESP.getFlashChipSize());
            log_i("Flash (SPI): %u B", spi_flash_get_chip_size());
            log_i("IRAM: %u B", ESP.getHeapSize());
#ifdef BOARD_HAS_PSRAM
            log_i("PSRAM: %u B", ESP.getPsramSize() + esp_himem_get_phys_size());
            log_i("SPIRAM: %u B", ESP.getPsramSize());
            log_i("HiMem Phys Size: %u B", esp_himem_get_phys_size());
            log_i("HiMem Reserved: %u B", esp_himem_reserved_area_size());
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
            log_v("Current system time: %s", buf);
            log_v("Getting time via SNTP");
            sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_setservername(0, (char*)"pool.ntp.org");
            sntp_init();
            delay(500);
            while (time(nullptr) < (2020 - 1970) * 365 * 24 * 3600)
                delay(1000);
            time(&now);
            nowtm = gmtime(&now);
            strftime(buf, 35, "%a, %d %b %Y %H:%M:%S GMT", nowtm);
            log_v("System time updated to: %s", buf);

            //Notify main task that time is set and it can proceed
            xTaskNotifyGive(taskHandles.taskMain);

            //Initialization done, delete task
            taskHandles.taskInit = NULL;
            vTaskDelete(NULL);
            log_e("Task failed to delete (shouldn't ever get here)");
        }

    }
}
