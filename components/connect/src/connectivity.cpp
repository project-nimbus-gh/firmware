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
        ESP_LOGW(kTag, "Connectivity stack already initialized");
        return;
    }

    ESP_LOGI(kTag, "Initializing connectivity stack...");
    
    initializeNvs();
    ESP_LOGI(kTag, "NVS initialized");
    
    initializeIfNeeded(esp_netif_init());
    ESP_LOGI(kTag, "Network interface initialized");
    
    initializeIfNeeded(esp_event_loop_create_default());
    ESP_LOGI(kTag, "Event loop initialized");

    wifi_.initialize();
    ESP_LOGI(kTag, "WiFi initialized");

    bluetooth_.initialize(BluetoothMode::DUAL);
    ESP_LOGI(kTag, "Bluetooth initialized");

    initialized_ = true;
    ESP_LOGI(kTag, "Connectivity stack initialized successfully");
}

void ConnectivityStack::shutdown()
{
    if (!initialized_) {
        ESP_LOGW(kTag, "Connectivity stack not initialized");
        return;
    }

    ESP_LOGI(kTag, "Shutting down connectivity stack...");

    bluetooth_.shutdown();
    ESP_LOGI(kTag, "Bluetooth shut down");

    wifi_.shutdown();
    ESP_LOGI(kTag, "WiFi shut down");

    initialized_ = false;
    ESP_LOGI(kTag, "Connectivity stack shut down successfully");
}

}  // namespace nimbus::connect
