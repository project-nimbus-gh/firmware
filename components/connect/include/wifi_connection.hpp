#pragma once

namespace nimbus::connect {

class WifiConnection final {
public:
    void start();

private:
    bool started_ {false};
};

}  // namespace nimbus::connect