#include "provisioning_manager.hpp"

#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace nimbus::provisioning {
namespace {

constexpr const char *kTag = "nimbus_setup";
constexpr const char *kNvsNamespace = "nimbus_setup";
constexpr const char *kPasswordKey = "admin_password";
constexpr const char *kWifiNamespace = "nimbus_wifi";
constexpr const char *kWifiSsidKey = "ssid";
constexpr const char *kWifiPasswordKey = "password";
constexpr const char *kDefaultPassword = "default";
constexpr uint16_t kServiceUuid = 0xFFF0;
constexpr uint16_t kRxCharUuid = 0xFFF1;
constexpr uint16_t kTxCharUuid = 0xFFF2;
constexpr uint16_t kAppId = 0x42;

struct BleState {
    esp_gatt_if_t gatts_if {ESP_GATT_IF_NONE};
    uint16_t conn_id {0};
    uint16_t service_handle {0};
    uint16_t rx_handle {0};
    uint16_t tx_handle {0};
    uint16_t tx_cccd_handle {0};
    bool service_ready {false};
    bool device_connected {false};
    bool notify_enabled {false};
    bool advertising_requested {false};
    bool advertising_started {false};
    bool rx_added {false};
    bool tx_added {false};
};

struct ProvisioningRequest {
    std::string op;
    std::optional<std::string> password;
    std::optional<std::string> new_password;
    std::optional<std::string> ssid;
    std::optional<std::string> wifi_password;
};

ProvisioningManager *g_instance = nullptr;

BleState &bleState()
{
    static BleState state;
    return state;
}

void initializeNvs()
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }

    ESP_ERROR_CHECK(result);
}

std::optional<std::string> readStringFromNvs(const char *namespace_name, const char *key)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (result != ESP_OK) {
        return std::nullopt;
    }

    char buffer[65] = {0};
    size_t length = sizeof(buffer);
    result = nvs_get_str(handle, key, buffer, &length);
    nvs_close(handle);

    if (result != ESP_OK || buffer[0] == '\0') {
        return std::nullopt;
    }

    return std::string(buffer);
}

void writeStringToNvs(const char *namespace_name, const char *key, const std::string &value)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        ESP_LOGW(kTag, "Failed to open NVS namespace '%s': %s", namespace_name, esp_err_to_name(result));
        return;
    }

    nvs_set_str(handle, key, value.c_str());
    nvs_commit(handle);
    nvs_close(handle);
}

void eraseKey(const char *namespace_name, const char *key)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        ESP_LOGW(kTag, "Failed to open NVS namespace '%s': %s", namespace_name, esp_err_to_name(result));
        return;
    }

    nvs_erase_key(handle, key);
    nvs_commit(handle);
    nvs_close(handle);
}

std::string jsonEscape(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (const char character : value) {
        switch (character) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }

    return escaped;
}

std::string jsonStringField(const char *key, const std::string &value)
{
    return std::string("\"") + key + "\":\"" + jsonEscape(value) + "\"";
}

std::string jsonBoolField(const char *key, bool value)
{
    return std::string("\"") + key + "\":" + (value ? "true" : "false");
}

std::string jsonNumberField(const char *key, double value)
{
    std::ostringstream stream;
    stream << value;
    return std::string("\"") + key + "\":" + stream.str();
}

std::string buildJsonObject(const std::vector<std::string> &fields)
{
    std::string json = "{";
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (index > 0) {
            json += ',';
        }
        json += fields[index];
    }
    json += '}';
    return json;
}

std::string buildJsonObject(std::initializer_list<std::string> fields)
{
    return buildJsonObject(std::vector<std::string>(fields));
}

std::string makeError(const std::string &message)
{
    return buildJsonObject({jsonBoolField("ok", false), jsonStringField("error", message)});
}

std::string makeAck(const std::string &message)
{
    return buildJsonObject({jsonBoolField("ok", true), jsonStringField("message", message)});
}

std::optional<std::string> extractJsonString(const std::string &payload, const std::string &key)
{
    const std::string needle = "\"" + key + "\"";
    const std::size_t key_position = payload.find(needle);
    if (key_position == std::string::npos) {
        return std::nullopt;
    }

    const std::size_t colon_position = payload.find(':', key_position + needle.size());
    if (colon_position == std::string::npos) {
        return std::nullopt;
    }

    std::size_t quote_position = payload.find('"', colon_position);
    if (quote_position == std::string::npos) {
        return std::nullopt;
    }

    ++quote_position;
    std::string value;
    bool escape = false;
    for (std::size_t index = quote_position; index < payload.size(); ++index) {
        const char character = payload[index];
        if (escape) {
            switch (character) {
                case '\\':
                    value.push_back('\\');
                    break;
                case '"':
                    value.push_back('"');
                    break;
                case '/':
                    value.push_back('/');
                    break;
                case 'b':
                    value.push_back('\b');
                    break;
                case 'f':
                    value.push_back('\f');
                    break;
                case 'n':
                    value.push_back('\n');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                default:
                    value.push_back(character);
                    break;
            }
            escape = false;
            continue;
        }

        if (character == '\\') {
            escape = true;
            continue;
        }

        if (character == '"') {
            return value;
        }

        value.push_back(character);
    }

    return std::nullopt;
}

std::optional<ProvisioningRequest> parseRequest(const std::string &payload)
{
    const std::optional<std::string> op = extractJsonString(payload, "op");
    if (!op.has_value()) {
        return std::nullopt;
    }

    ProvisioningRequest request;
    request.op = *op;

    request.password = extractJsonString(payload, "password");
    request.new_password = extractJsonString(payload, "new_password");
    request.ssid = extractJsonString(payload, "ssid");
    request.wifi_password = extractJsonString(payload, "wifi_password");
    return request;
}

void sendResponse(const std::string &payload)
{
    BleState &state = bleState();
    if (!state.device_connected || !state.notify_enabled || state.gatts_if == ESP_GATT_IF_NONE || state.tx_handle == 0) {
        ESP_LOGI(kTag, "BLE response: %s", payload.c_str());
        return;
    }

    std::vector<uint8_t> bytes(payload.begin(), payload.end());
    esp_err_t result = esp_ble_gatts_send_indicate(
        state.gatts_if,
        state.conn_id,
        state.tx_handle,
        static_cast<uint16_t>(bytes.size()),
        bytes.data(),
        false);

    if (result != ESP_OK) {
        ESP_LOGW(kTag, "Failed to send BLE response: %s", esp_err_to_name(result));
    }
}

std::string wifiStateToString(nimbus::connect::WifiState state)
{
    switch (state) {
        case nimbus::connect::WifiState::DISCONNECTED:
            return "disconnected";
        case nimbus::connect::WifiState::CONNECTING:
            return "connecting";
        case nimbus::connect::WifiState::CONNECTED:
            return "connected";
        case nimbus::connect::WifiState::RECONNECTING:
            return "reconnecting";
        case nimbus::connect::WifiState::FAILED:
            return "failed";
    }

    return "unknown";
}

}  // namespace

void ProvisioningManager::start()
{
    if (g_instance != nullptr) {
        ESP_LOGW(kTag, "Provisioning manager already running");
        return;
    }

    g_instance = this;
    initializeStorage();
    initializeWifi();
    connectSavedWifi();
    initializeBluetooth();
}

void ProvisioningManager::initializeStorage()
{
    initializeNvs();
    loadAdminPassword();
}

void ProvisioningManager::loadAdminPassword()
{
    const std::optional<std::string> stored_password = readStringFromNvs(kNvsNamespace, kPasswordKey);
    if (stored_password.has_value()) {
        admin_password_ = *stored_password;
        must_change_password_ = admin_password_ == kDefaultPassword;
        return;
    }

    admin_password_ = kDefaultPassword;
    must_change_password_ = true;
}

void ProvisioningManager::saveAdminPassword(const std::string &password)
{
    admin_password_ = password;
    must_change_password_ = false;
    writeStringToNvs(kNvsNamespace, kPasswordKey, password);
}

void ProvisioningManager::connectSavedWifi()
{
    if (!wifi_.connectSaved()) {
        ESP_LOGI(kTag, "No saved Wi-Fi credentials yet");
    }
}

void ProvisioningManager::initializeWifi()
{
    wifi_.initialize();
    wifi_.setStateCallback([](nimbus::connect::WifiState state) {
        ESP_LOGI(kTag, "Wi-Fi state: %s", wifiStateToString(state).c_str());
    });
}

void ProvisioningManager::restartDevice()
{
    ESP_LOGI(kTag, "Restart requested over BLE");
    sendResponse(makeAck("restart scheduled"));
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
}

void ProvisioningManager::factoryReset()
{
    ESP_LOGW(kTag, "Factory reset requested over BLE");
    eraseKey(kWifiNamespace, kWifiSsidKey);
    eraseKey(kWifiNamespace, kWifiPasswordKey);
    eraseKey(kNvsNamespace, kPasswordKey);
    sendResponse(makeAck("factory reset complete"));
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
}

void ProvisioningManager::forgetWifi()
{
    ESP_LOGI(kTag, "Forgetting stored Wi-Fi credentials");
    wifi_.disconnect();
    eraseKey(kWifiNamespace, kWifiSsidKey);
    eraseKey(kWifiNamespace, kWifiPasswordKey);
    sendResponse(makeAck("wifi credentials erased"));
}

void ProvisioningManager::initializeBluetooth()
{
    esp_err_t result = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(kTag, "Classic BT memory release returned: %s", esp_err_to_name(result));
    }

    esp_bt_controller_config_t controller_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&controller_config));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_ERROR_CHECK(esp_ble_gatt_set_local_mtu(517));

    ESP_ERROR_CHECK(esp_ble_gap_register_callback([](esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
        if (g_instance != nullptr) {
            g_instance->onGapEvent(event, param);
        }
    }));

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback([](esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
        if (g_instance != nullptr) {
            g_instance->onGattsEvent(event, gatts_if, param);
        }
    }));

    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(kDeviceName));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(kAppId));

    ESP_LOGI(kTag, "Bluetooth provisioning initialized as %s", kDeviceName);
}

void ProvisioningManager::beginAdvertising()
{
    BleState &state = bleState();
    if (!state.service_ready) {
        return;
    }

    esp_ble_adv_data_t adv_data = {};
    adv_data.set_scan_rsp = false;
    adv_data.include_name = true;
    adv_data.include_txpower = true;
    adv_data.appearance = 0x0000;
    adv_data.manufacturer_len = 0;
    adv_data.p_manufacturer_data = nullptr;
    adv_data.service_data_len = 0;
    adv_data.p_service_data = nullptr;
    adv_data.service_uuid_len = 0;
    adv_data.p_service_uuid = nullptr;
    adv_data.flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT;

    advertising_params_.adv_int_min = 0x20;
    advertising_params_.adv_int_max = 0x40;
    advertising_params_.adv_type = ADV_TYPE_IND;
    advertising_params_.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    advertising_params_.channel_map = ADV_CHNL_ALL;
    advertising_params_.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

    state.advertising_requested = true;
    if (state.advertising_started) {
        ESP_ERROR_CHECK(esp_ble_gap_stop_advertising());
        state.advertising_started = false;
    }

    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));
}

void ProvisioningManager::onGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            if (bleState().advertising_requested && !bleState().advertising_started) {
                ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&advertising_params_));
                bleState().advertising_started = true;
            }
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGW(kTag, "BLE advertising failed to start: %d", param->adv_start_cmpl.status);
            } else {
                ESP_LOGI(kTag, "BLE advertising started");
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(kTag, "BLE advertising stopped");
            break;

        default:
            break;
    }
}

void ProvisioningManager::onGattsEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            BleState &state = bleState();
            state.gatts_if = gatts_if;

            esp_gatt_srvc_id_t service_id = {};
            service_id.is_primary = true;
            service_id.id.inst_id = 0;
            service_id.id.uuid.len = ESP_UUID_LEN_16;
            service_id.id.uuid.uuid.uuid16 = kServiceUuid;

            ESP_ERROR_CHECK(esp_ble_gatts_create_service(gatts_if, &service_id, 6));
            break;
        }

        case ESP_GATTS_CREATE_EVT: {
            BleState &state = bleState();
            state.service_handle = param->create.service_handle;
            ESP_ERROR_CHECK(esp_ble_gatts_start_service(state.service_handle));

            esp_bt_uuid_t rx_uuid = {};
            rx_uuid.len = ESP_UUID_LEN_16;
            rx_uuid.uuid.uuid16 = kRxCharUuid;

            uint8_t rx_initial_value[1] = {0};
            esp_attr_value_t rx_attr_value = {};
            rx_attr_value.attr_max_len = 512;
            rx_attr_value.attr_len = 1;
            rx_attr_value.attr_value = rx_initial_value;

            ESP_ERROR_CHECK(esp_ble_gatts_add_char(
                state.service_handle,
                &rx_uuid,
                ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR,
                &rx_attr_value,
                nullptr));
            break;
        }

        case ESP_GATTS_ADD_CHAR_EVT: {
            BleState &state = bleState();
            if (!state.rx_added) {
                state.rx_added = true;
                state.rx_handle = param->add_char.attr_handle;

                esp_bt_uuid_t tx_uuid = {};
                tx_uuid.len = ESP_UUID_LEN_16;
                tx_uuid.uuid.uuid16 = kTxCharUuid;

                uint8_t tx_initial_value[1] = {0};
                esp_attr_value_t tx_attr_value = {};
                tx_attr_value.attr_max_len = 512;
                tx_attr_value.attr_len = 1;
                tx_attr_value.attr_value = tx_initial_value;

                ESP_ERROR_CHECK(esp_ble_gatts_add_char(
                    state.service_handle,
                    &tx_uuid,
                    ESP_GATT_PERM_READ,
                    ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ,
                    &tx_attr_value,
                    nullptr));
            } else {
                state.tx_added = true;
                state.tx_handle = param->add_char.attr_handle;

                esp_bt_uuid_t cccd_uuid = {};
                cccd_uuid.len = ESP_UUID_LEN_16;
                cccd_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

                uint8_t cccd_value[2] = {0x00, 0x00};
                esp_attr_value_t cccd_attr_value = {};
                cccd_attr_value.attr_max_len = 2;
                cccd_attr_value.attr_len = 2;
                cccd_attr_value.attr_value = cccd_value;

                ESP_ERROR_CHECK(esp_ble_gatts_add_char_descr(
                    state.service_handle,
                    &cccd_uuid,
                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                    &cccd_attr_value,
                    nullptr));
            }
            break;
        }

        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            bleState().tx_cccd_handle = param->add_char_descr.attr_handle;
            bleState().service_ready = true;
            bleState().advertising_requested = true;
            ESP_LOGI(kTag, "BLE GATT service ready");
            beginAdvertising();
            break;

        case ESP_GATTS_CONNECT_EVT:
            bleState().device_connected = true;
            bleState().conn_id = param->connect.conn_id;
            g_instance->authenticated_ = false;
            ESP_LOGI(kTag, "BLE client connected");
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            bleState().device_connected = false;
            bleState().notify_enabled = false;
            bleState().advertising_started = false;
            g_instance->authenticated_ = false;
            ESP_LOGI(kTag, "BLE client disconnected");
            beginAdvertising();
            break;

        case ESP_GATTS_WRITE_EVT: {
            BleState &state = bleState();
            if (param->write.handle == state.tx_cccd_handle && param->write.len >= 2) {
                state.notify_enabled = (param->write.value[0] == 0x01 && param->write.value[1] == 0x00);
                ESP_LOGI(kTag, "BLE notifications %s", state.notify_enabled ? "enabled" : "disabled");
                break;
            }

            if (param->write.handle != state.rx_handle) {
                break;
            }

            std::string payload(reinterpret_cast<const char *>(param->write.value), param->write.len);
            const std::optional<ProvisioningRequest> request = parseRequest(payload);
            if (!request.has_value()) {
                sendResponse(makeError("invalid json"));
                break;
            }

            if (request->op == "auth") {
                if (!request->password.has_value()) {
                    sendResponse(makeError("missing password"));
                    break;
                }

                if (g_instance->admin_password_ != request->password.value()) {
                    sendResponse(makeError("invalid password"));
                    break;
                }

                g_instance->authenticated_ = true;
                sendResponse(buildJsonObject({
                    jsonBoolField("ok", true),
                    jsonStringField("message", "authenticated"),
                    jsonBoolField("must_change_password", g_instance->must_change_password_),
                }));
            } else if (request->op == "change_password") {
                if (!g_instance->authenticated_) {
                    sendResponse(makeError("authenticate first"));
                    break;
                }

                if (!request->new_password.has_value()) {
                    sendResponse(makeError("missing new_password"));
                    break;
                }

                const std::string new_password = request->new_password.value();
                if (new_password.empty() || new_password.size() > 64) {
                    sendResponse(makeError("invalid new_password length"));
                    break;
                }

                g_instance->saveAdminPassword(new_password);
                sendResponse(buildJsonObject({
                    jsonBoolField("ok", true),
                    jsonStringField("message", "password updated"),
                    jsonBoolField("must_change_password", g_instance->must_change_password_),
                }));
            } else if (request->op == "wifi") {
                if (g_instance->must_change_password_) {
                    sendResponse(makeError("change password first"));
                    break;
                }

                if (!g_instance->authenticated_) {
                    sendResponse(makeError("authenticate first"));
                    break;
                }

                if (!request->ssid.has_value() || !request->wifi_password.has_value()) {
                    sendResponse(makeError("missing ssid or password"));
                    break;
                }

                bool started = g_instance->wifi_.connect(request->ssid.value(), request->wifi_password.value());
                if (!started) {
                    sendResponse(makeError("wifi connect failed"));
                    break;
                }

                sendResponse(buildJsonObject({
                    jsonBoolField("ok", true),
                    jsonStringField("message", "wifi provisioning started"),
                    jsonStringField("ssid", request->ssid.value()),
                }));
            } else if (request->op == "status" || request->op == "debug") {
                if (g_instance->must_change_password_) {
                    sendResponse(makeError("change password first"));
                    break;
                }

                if (!g_instance->authenticated_) {
                    sendResponse(makeError("authenticate first"));
                    break;
                }

                std::vector<std::string> fields {
                    jsonBoolField("ok", true),
                    jsonStringField("wifi_state", wifiStateToString(g_instance->wifi_.getState())),
                    jsonStringField("wifi_ssid", g_instance->wifi_.getConnectedSSID()),
                    jsonBoolField("authenticated", g_instance->authenticated_),
                    jsonBoolField("must_change_password", g_instance->must_change_password_),
                    jsonNumberField("free_heap", static_cast<double>(esp_get_free_heap_size())),
                };

                if (request->op == "debug") {
                    fields.push_back(jsonNumberField("wifi_signal_dbm", static_cast<double>(g_instance->wifi_.getSignalStrength())));
                    fields.push_back(jsonBoolField("notify_enabled", bleState().notify_enabled));
                }

                sendResponse(buildJsonObject(fields));
            } else if (request->op == "restart") {
                if (g_instance->must_change_password_) {
                    sendResponse(makeError("change password first"));
                    break;
                }

                if (!g_instance->authenticated_) {
                    sendResponse(makeError("authenticate first"));
                    break;
                }

                g_instance->restartDevice();
                break;
            } else if (request->op == "forget_wifi") {
                if (g_instance->must_change_password_) {
                    sendResponse(makeError("change password first"));
                    break;
                }

                if (!g_instance->authenticated_) {
                    sendResponse(makeError("authenticate first"));
                    break;
                }

                g_instance->forgetWifi();
                break;
            } else if (request->op == "factory_reset") {
                if (g_instance->must_change_password_) {
                    sendResponse(makeError("change password first"));
                    break;
                }

                if (!g_instance->authenticated_) {
                    sendResponse(makeError("authenticate first"));
                    break;
                }

                g_instance->factoryReset();
                break;
            } else {
                sendResponse(makeError("unknown op"));
                break;
            }

            break;
        }

        default:
            break;
    }
}

}  // namespace nimbus::provisioning
