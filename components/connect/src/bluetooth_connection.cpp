#include "bluetooth_connection.hpp"

#include "esp_err.h"
#include "esp_log.h"

namespace nimbus::connect {
namespace {

constexpr const char *kTag = "nimbus_bt";

BluetoothConnection *g_bt_instance = nullptr;

}  // namespace

void BluetoothConnection::initialize(BluetoothMode mode)
{
    if (initialized_) {
        ESP_LOGW(kTag, "Bluetooth already initialized");
        return;
    }

    g_bt_instance = this;
    mode_ = mode;

    initializeController(mode);
    initializeStack(mode);
    initialized_ = true;
    updateState(BluetoothState::DISCONNECTED);
    ESP_LOGI(kTag, "Bluetooth initialized successfully");
}

bool BluetoothConnection::startAdvertisement(const std::string &device_name, bool include_tx_power)
{
    if (!initialized_) {
        ESP_LOGE(kTag, "Bluetooth not initialized");
        return false;
    }

    if (mode_ == BluetoothMode::CLASSIC) {
        ESP_LOGE(kTag, "BLE advertisement not supported in CLASSIC mode");
        return false;
    }

    device_name_ = device_name;
    advertising_ = true;
    ESP_LOGI(kTag, "BLE advertisement started: %s", device_name.c_str());
    return true;
}

void BluetoothConnection::stopAdvertisement()
{
    if (!advertising_) {
        return;
    }

    advertising_ = false;
    ESP_LOGI(kTag, "BLE advertisement stopped");
}

void BluetoothConnection::startScan(uint32_t duration_seconds)
{
    if (!initialized_) {
        ESP_LOGE(kTag, "Bluetooth not initialized");
        return;
    }

    if (mode_ == BluetoothMode::CLASSIC) {
        ESP_LOGE(kTag, "BLE scan not supported in CLASSIC mode");
        return;
    }

    scanning_ = true;
    ESP_LOGI(kTag, "BLE scan started for %u seconds", duration_seconds);
}

void BluetoothConnection::stopScan()
{
    if (!scanning_) {
        return;
    }

    scanning_ = false;
    ESP_LOGI(kTag, "BLE scan stopped");
}

bool BluetoothConnection::connectDevice(const std::string &address)
{
    if (!initialized_) {
        ESP_LOGE(kTag, "Bluetooth not initialized");
        return false;
    }

    if (address.empty()) {
        ESP_LOGE(kTag, "Invalid device address");
        return false;
    }

    connected_device_address_ = address;
    updateState(BluetoothState::CONNECTING);
    ESP_LOGI(kTag, "Initiating connection to device: %s", address.c_str());
    return true;
}

void BluetoothConnection::disconnectDevice()
{
    if (state_ != BluetoothState::CONNECTED && state_ != BluetoothState::CONNECTING) {
        return;
    }

    connected_device_address_.clear();
    updateState(BluetoothState::DISCONNECTED);
    ESP_LOGI(kTag, "Device disconnected");
}

bool BluetoothConnection::sendData(const std::vector<uint8_t> &data)
{
    if (state_ != BluetoothState::CONNECTED) {
        ESP_LOGE(kTag, "Cannot send data: not connected");
        return false;
    }

    if (data.empty()) {
        ESP_LOGE(kTag, "Cannot send empty data");
        return false;
    }

    ESP_LOGI(kTag, "Sending %zu bytes", data.size());
    return true;
}

void BluetoothConnection::shutdown()
{
    if (!initialized_) {
        return;
    }

    if (advertising_) {
        stopAdvertisement();
    }

    if (scanning_) {
        stopScan();
    }

    if (isConnected()) {
        disconnectDevice();
    }

    releaseResources();
    initialized_ = false;
    g_bt_instance = nullptr;
    ESP_LOGI(kTag, "Bluetooth shut down successfully");
}

void BluetoothConnection::initializeController(BluetoothMode mode)
{
    ESP_LOGI(kTag, "Initializing Bluetooth controller in mode: %d", static_cast<int>(mode));
}

void BluetoothConnection::initializeStack(BluetoothMode mode)
{
    ESP_LOGI(kTag, "Initializing Bluetooth stack");
}

void BluetoothConnection::releaseResources()
{
    ESP_LOGI(kTag, "Releasing Bluetooth resources");
}

void BluetoothConnection::updateState(BluetoothState newState)
{
    if (state_ != newState) {
        state_ = newState;

        if (connection_callback_) {
            connection_callback_(state_);
        }

        switch (state_) {
            case BluetoothState::CONNECTED:
                ESP_LOGI(kTag, "Bluetooth state: CONNECTED");
                break;
            case BluetoothState::DISCONNECTED:
                ESP_LOGI(kTag, "Bluetooth state: DISCONNECTED");
                break;
            case BluetoothState::CONNECTING:
                ESP_LOGI(kTag, "Bluetooth state: CONNECTING");
                break;
            case BluetoothState::DISCONNECTING:
                ESP_LOGI(kTag, "Bluetooth state: DISCONNECTING");
                break;
            case BluetoothState::ERROR:
                ESP_LOGE(kTag, "Bluetooth state: ERROR");
                break;
        }
    }
}

void BluetoothConnection::registerGapCallbacks()
{
    // Placeholder for additional GAP callbacks
}

void BluetoothConnection::registerGattCallbacks()
{
    // Placeholder for GATT callbacks
}

}  // namespace nimbus::connect
