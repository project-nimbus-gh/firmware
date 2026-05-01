#include "connectivity.hpp"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

namespace nimbus::connect {
namespace {

constexpr const char *kTag = "nimbus_connect";

void initializeNvs()
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }

    ESP_ERROR_CHECK(result);
}

void initializeIfNeeded(esp_err_t result)
{
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(result);
    }
}

}  // namespace

void ConnectivityStack::initialize()
{
    if (initialized_) {
        return;
    }

    initializeNvs();
    initializeIfNeeded(esp_netif_init());
    initializeIfNeeded(esp_event_loop_create_default());

    wifi_.start();

    initialized_ = true;
    ESP_LOGI(kTag, "Connectivity stack initialized");
}

}  // namespace nimbus::connect
