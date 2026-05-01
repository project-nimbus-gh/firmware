#pragma once

#include "driver/gpio.h"

namespace nimbus::driver {

class GpioOutput final {
public:
    explicit GpioOutput(gpio_num_t pin) noexcept;

    void initialize() const;
    void set(bool enabled) const;

    [[nodiscard]] gpio_num_t pin() const noexcept;

private:
    gpio_num_t pin_;
};

}  // namespace nimbus::driver
