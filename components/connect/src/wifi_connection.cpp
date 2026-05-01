#include "wifi_connection.hpp"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

namespace nimbus::connect {
namespace {

constexpr const char *kTag = "nimbus_wifi";

}  // namespace

void WifiConnection::start()
{
    if (started_) {
        return;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    started_ = true;
    ESP_LOGI(kTag, "Wi-Fi station stack started");
}

}  // namespace nimbus::connect
