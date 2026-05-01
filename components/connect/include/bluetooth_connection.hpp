#pragma once

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include <string>
#include <functional>
#include <cstdint>
#include <vector>

namespace nimbus::connect {

/**
 * @brief Bluetooth connection state enumeration
 */
enum class BluetoothState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    ERROR
};

/**
 * @brief Supported Bluetooth modes
 */
enum class BluetoothMode {
    CLASSIC,  ///< Bluetooth Classic (BR/EDR)
    BLE,      ///< Bluetooth Low Energy
    DUAL      ///< Both Classic and BLE
};

/**
 * @brief Bluetooth event callback function signatures
 */
using BluetoothConnectionCallback = std::function<void(BluetoothState state)>;
using BluetoothDataCallback = std::function<void(const std::vector<uint8_t> &data)>;

/**
 * @brief Manages Bluetooth connectivity for the Nimbus device
 * 
 * Handles initialization and management of Bluetooth Classic (BR/EDR)
 * and Bluetooth Low Energy (BLE) connections with GATT support.
 */
class BluetoothConnection final {
public:
    /**
     * @brief Initialize Bluetooth with specified mode
     * @param mode The Bluetooth mode to initialize
     */
    void initialize(BluetoothMode mode = BluetoothMode::DUAL);

    /**
     * @brief Start BLE advertisement
     * @param device_name Name for the device
     * @param include_tx_power Include TX power in advertisement
     * @return true if advertisement started successfully
     */
    bool startAdvertisement(const std::string &device_name, bool include_tx_power = true);

    /**
     * @brief Stop BLE advertisement
     */
    void stopAdvertisement();

    /**
     * @brief Start BLE scan for discovery
     * @param duration_seconds Scan duration in seconds (0 = continuous)
     */
    void startScan(uint32_t duration_seconds = 0);

    /**
     * @brief Stop BLE scan
     */
    void stopScan();

    /**
     * @brief Connect to a BLE device
     * @param address Device MAC address
     * @return true if connection initiated successfully
     */
    bool connectDevice(const std::string &address);

    /**
     * @brief Disconnect from connected device
     */
    void disconnectDevice();

    /**
     * @brief Send data to connected device
     * @param data Data buffer to send
     * @return true if data sent successfully
     */
    bool sendData(const std::vector<uint8_t> &data);

    /**
     * @brief Shutdown Bluetooth
     */
    void shutdown();

    /**
     * @brief Check if Bluetooth is initialized
     * @return true if Bluetooth is initialized and ready, false otherwise
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Check if device is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const { return state_ == BluetoothState::CONNECTED; }

    /**
     * @brief Get the current Bluetooth mode
     * @return The current BluetoothMode
     */
    BluetoothMode getMode() const { return mode_; }

    /**
     * @brief Get current connection state
     * @return Current BluetoothState
     */
    BluetoothState getState() const { return state_; }

    /**
     * @brief Register connection state callback
     * @param callback Function to call on state changes
     */
    void setConnectionCallback(BluetoothConnectionCallback callback) {
        connection_callback_ = callback;
    }

    /**
     * @brief Register data received callback
     * @param callback Function to call when data is received
     */
    void setDataCallback(BluetoothDataCallback callback) {
        data_callback_ = callback;
    }

    /**
     * @brief Get connected device address
     * @return Device MAC address or empty string if not connected
     */
    std::string getConnectedDeviceAddress() const { return connected_device_address_; }

    /**
     * @brief Get device name
     * @return Current device name
     */
    std::string getDeviceName() const { return device_name_; }

private:
    bool initialized_ {false};
    BluetoothMode mode_ {BluetoothMode::DUAL};
    BluetoothState state_ {BluetoothState::DISCONNECTED};
    std::string device_name_ {"Nimbus"};
    std::string connected_device_address_;
    bool advertising_ {false};
    bool scanning_ {false};
    
    BluetoothConnectionCallback connection_callback_;
    BluetoothDataCallback data_callback_;

    /**
     * @brief Initialize Bluetooth controller
     */
    void initializeController(BluetoothMode mode);

    /**
     * @brief Initialize Bluetooth stack
     */
    void initializeStack(BluetoothMode mode);

    /**
     * @brief Release Bluetooth memory and resources
     */
    void releaseResources();

    /**
     * @brief Update connection state and trigger callback
     */
    void updateState(BluetoothState newState);

    /**
     * @brief Register BLE GAP callbacks
     */
    void registerGapCallbacks();

    /**
     * @brief Register BLE GATT callbacks
     */
    void registerGattCallbacks();
};

}  // namespace nimbus::connect
