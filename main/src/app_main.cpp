#include <cstddef>
#include <cstdio>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "provisioning_manager.hpp"

namespace {

constexpr const char *kTag = "nimbus_firmware";

}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "Starting Nimbus provisioning firmware");

    nimbus::provisioning::ProvisioningManager provisioning_manager;
    provisioning_manager.start();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
