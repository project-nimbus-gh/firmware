#pragma once

#include <cstdint>
#include <string>

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "wifi_connection.hpp"

namespace nimbus::provisioning {

class ProvisioningManager final {
public:
    void start();

private:
    static constexpr const char *kDeviceName {"Nimbus-Setup"};

    nimbus::connect::WifiConnection wifi_;
    std::string admin_password_ {"default"};
    bool must_change_password_ {true};
    bool authenticated_ {false};
    esp_ble_adv_params_t advertising_params_ {};

    void initializeStorage();
    void loadAdminPassword();
    void saveAdminPassword(const std::string &password);
    void connectSavedWifi();
    void initializeWifi();
    void initializeBluetooth();
    void beginAdvertising();
    void onGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    void onGattsEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    void restartDevice();
    void factoryReset();
    void forgetWifi();
};

}  // namespace nimbus::provisioning
