#pragma once

#include <string>
#include <functional>
#include <cstdint>

namespace nimbus::connect {

/**
 * @brief WiFi connection state enumeration
 */
enum class WifiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    FAILED
};

/**
 * @brief WiFi event callback function signature
 */
using WifiEventCallback = std::function<void(WifiState state)>;

/**
 * @brief Manages WiFi connectivity for the Nimbus device
 * 
 * Provides WiFi station mode with connection management, event handling,
 * and persistent credential storage.
 */
class WifiConnection final {
public:
    /**
     * @brief Initialize WiFi subsystem
     */
    void initialize();

    /**
     * @brief Connect to a WiFi network with SSID and password
     * @param ssid Network SSID
     * @param password Network password
     * @return true if connection initiated successfully, false otherwise
     */
    bool connect(const std::string &ssid, const std::string &password);

    /**
     * @brief Connect to a WiFi network using saved credentials
     * @return true if connection initiated successfully, false otherwise
     */
    bool connectSaved();

    /**
     * @brief Disconnect from WiFi network
     */
    void disconnect();

    /**
     * @brief Check current WiFi connection state
     * @return Current WifiState
     */
    WifiState getState() const { return state_; }

    /**
     * @brief Check if WiFi is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const { return state_ == WifiState::CONNECTED; }

    /**
     * @brief Register a callback for WiFi state changes
     * @param callback Function to call on state changes
     */
    void setStateCallback(WifiEventCallback callback) { state_callback_ = callback; }

    /**
     * @brief Get connected network SSID
     * @return SSID string or empty if not connected
     */
    std::string getConnectedSSID() const { return connected_ssid_; }

    /**
     * @brief Get signal strength in dBm
     * @return Signal strength value or 0 if not connected
     */
    int8_t getSignalStrength() const { return signal_strength_; }

    /**
     * @brief Shutdown WiFi
     */
    void shutdown();

    /**
     * @brief Set WiFi state (used internally by event handlers)
     * @param newState The new WiFi state
     */
    void setWifiState(WifiState newState) { updateState(newState); }

protected:
    /**
     * @brief Update WiFi state and trigger callback
     */
    void updateState(WifiState newState);

private:
    WifiState state_ {WifiState::DISCONNECTED};
    bool initialized_ {false};
    std::string saved_ssid_;
    std::string saved_password_;
    std::string connected_ssid_;
    int8_t signal_strength_ {0};
    WifiEventCallback state_callback_;

    /**
     * @brief Save WiFi credentials to NVS
     */
    void saveCredentials(const std::string &ssid, const std::string &password);

    /**
     * @brief Load WiFi credentials from NVS
     * @return true if credentials were loaded, false otherwise
     */
    bool loadCredentials();
};

}  // namespace nimbus::connect
