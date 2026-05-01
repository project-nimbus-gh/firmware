#include "wifi_connection.hpp"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"

namespace nimbus::connect {
namespace {

constexpr const char *kTag = "nimbus_wifi";
constexpr const char *kNvsNamespace = "nimbus_wifi";
constexpr const char *kNvsSsidKey = "ssid";
constexpr const char *kNvsPasswordKey = "password";
constexpr uint32_t kMaxRetries = 5;

WifiConnection *g_wifi_instance = nullptr;

void wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (g_wifi_instance == nullptr) {
        return;
    }

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(kTag, "WiFi station started, connecting...");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t *event = static_cast<wifi_event_sta_connected_t *>(event_data);
                ESP_LOGI(kTag, "Connected to SSID: %s", event->ssid);
                g_wifi_instance->setWifiState(WifiState::CONNECTING);
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *event = static_cast<wifi_event_sta_disconnected_t *>(event_data);
                ESP_LOGW(kTag, "Disconnected (reason: %d)", event->reason);
                if (event->reason == WIFI_REASON_NO_AP_FOUND || 
                    event->reason == WIFI_REASON_AUTH_FAIL ||
                    event->reason == WIFI_REASON_ASSOC_FAIL) {
                    g_wifi_instance->setWifiState(WifiState::FAILED);
                } else {
                    g_wifi_instance->setWifiState(WifiState::RECONNECTING);
                    esp_wifi_connect();
                }
                break;
            }

            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t *>(event_data);
        ESP_LOGI(kTag, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        g_wifi_instance->setWifiState(WifiState::CONNECTED);
    }
}

}  // namespace

void WifiConnection::initialize()
{
    if (initialized_) {
        ESP_LOGW(kTag, "WiFi already initialized");
        return;
    }

    g_wifi_instance = this;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    initialized_ = true;
    ESP_LOGI(kTag, "WiFi initialized successfully");

    loadCredentials();
}

bool WifiConnection::connect(const std::string &ssid, const std::string &password)
{
    if (!initialized_) {
        ESP_LOGE(kTag, "WiFi not initialized");
        return false;
    }

    if (ssid.empty() || ssid.length() > 32) {
        ESP_LOGE(kTag, "Invalid SSID length");
        return false;
    }

    if (password.length() > 64) {
        ESP_LOGE(kTag, "Invalid password length");
        return false;
    }

    saved_ssid_ = ssid;
    saved_password_ = password;
    saveCredentials(ssid, password);

    wifi_config_t wifi_config = {};
    memcpy(wifi_config.sta.ssid, ssid.c_str(), ssid.length());
    memcpy(wifi_config.sta.password, password.c_str(), password.length());
    wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

    updateState(WifiState::CONNECTING);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(kTag, "Connection initiated to SSID: %s", ssid.c_str());
    return true;
}

bool WifiConnection::connectSaved()
{
    if (!loadCredentials()) {
        ESP_LOGW(kTag, "No saved credentials found");
        return false;
    }

    return connect(saved_ssid_, saved_password_);
}

void WifiConnection::disconnect()
{
    if (!initialized_) {
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    updateState(WifiState::DISCONNECTED);
    ESP_LOGI(kTag, "WiFi disconnected");
}

void WifiConnection::shutdown()
{
    if (!initialized_) {
        return;
    }

    if (isConnected() || state_ == WifiState::CONNECTING) {
        disconnect();
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler));

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    initialized_ = false;
    g_wifi_instance = nullptr;
    ESP_LOGI(kTag, "WiFi shut down successfully");
}

void WifiConnection::saveCredentials(const std::string &ssid, const std::string &password)
{
    nvs_handle_t nvs_handle;
    esp_err_t result = nvs_open(kNvsNamespace, NVS_READWRITE, &nvs_handle);
    
    if (result != ESP_OK) {
        ESP_LOGW(kTag, "Failed to open NVS namespace: %s", esp_err_to_name(result));
        return;
    }

    nvs_set_str(nvs_handle, kNvsSsidKey, ssid.c_str());
    nvs_set_str(nvs_handle, kNvsPasswordKey, password.c_str());
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    ESP_LOGI(kTag, "Credentials saved to NVS");
}

bool WifiConnection::loadCredentials()
{
    nvs_handle_t nvs_handle;
    esp_err_t result = nvs_open(kNvsNamespace, NVS_READONLY, &nvs_handle);
    
    if (result != ESP_OK) {
        ESP_LOGD(kTag, "No saved credentials in NVS");
        return false;
    }

    char ssid[33] = {0};
    char password[65] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t password_len = sizeof(password);

    result = nvs_get_str(nvs_handle, kNvsSsidKey, ssid, &ssid_len);
    if (result != ESP_OK) {
        nvs_close(nvs_handle);
        return false;
    }

    result = nvs_get_str(nvs_handle, kNvsPasswordKey, password, &password_len);
    nvs_close(nvs_handle);
    
    if (result != ESP_OK) {
        return false;
    }

    saved_ssid_ = ssid;
    saved_password_ = password;
    ESP_LOGI(kTag, "Credentials loaded from NVS: %s", saved_ssid_.c_str());
    return true;
}

void WifiConnection::updateState(WifiState newState)
{
    if (state_ != newState) {
        state_ = newState;
        
        if (state_callback_) {
            state_callback_(state_);
        }

        switch (state_) {
            case WifiState::CONNECTED:
                connected_ssid_ = saved_ssid_;
                ESP_LOGI(kTag, "WiFi state changed to CONNECTED");
                break;
            case WifiState::DISCONNECTED:
                connected_ssid_.clear();
                ESP_LOGI(kTag, "WiFi state changed to DISCONNECTED");
                break;
            case WifiState::FAILED:
                ESP_LOGE(kTag, "WiFi state changed to FAILED");
                break;
            default:
                ESP_LOGD(kTag, "WiFi state changed to %d", static_cast<int>(state_));
                break;
        }
    }
}

}  // namespace nimbus::connect
