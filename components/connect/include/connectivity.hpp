#pragma once

#include "wifi_connection.hpp"
#include "bluetooth_connection.hpp"

namespace nimbus::connect {

/**
 * @brief Main connectivity stack managing WiFi and Bluetooth
 * 
 * Orchestrates initialization and lifecycle of all connectivity interfaces.
 */
class ConnectivityStack final {
public:
    /**
     * @brief Initialize the complete connectivity stack
     */
    void initialize();

    /**
     * @brief Shutdown the connectivity stack
     */
    void shutdown();

    /**
     * @brief Get WiFi connection interface
     * @return Reference to WifiConnection
     */
    WifiConnection &getWifi() { return wifi_; }

    /**
     * @brief Get Bluetooth connection interface
     * @return Reference to BluetoothConnection
     */
    BluetoothConnection &getBluetooth() { return bluetooth_; }

    /**
     * @brief Check if connectivity stack is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return initialized_; }

private:
    WifiConnection wifi_;
    BluetoothConnection bluetooth_;
    bool initialized_ {false};
};

}  // namespace nimbus::connect
