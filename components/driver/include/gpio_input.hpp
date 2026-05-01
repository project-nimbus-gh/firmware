#pragma once

#include "driver/gpio.h"

namespace nimbus::driver {

class GpioInput final {
public:
    explicit GpioInput(gpio_num_t pin, bool pull_up = false, bool pull_down = false) noexcept;

    void initialize() const;

    [[nodiscard]] int read() const;

    [[nodiscard]] gpio_num_t pin() const noexcept;

private:
    gpio_num_t pin_;
    bool pull_up_;
    bool pull_down_;
};

}  // namespace nimbus::driver
