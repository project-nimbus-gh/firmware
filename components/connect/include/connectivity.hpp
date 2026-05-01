#pragma once

#include "wifi_connection.hpp"

namespace nimbus::connect {

class ConnectivityStack final {
public:
    void initialize();

private:
    WifiConnection wifi_;
    bool initialized_ {false};
};

}  // namespace nimbus::connect
