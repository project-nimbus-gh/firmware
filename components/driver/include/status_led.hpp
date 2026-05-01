#pragma once

#include "gpio_output.hpp"

namespace nimbus::driver {

class StatusLed final {
public:
    explicit StatusLed(gpio_num_t pin = GPIO_NUM_2) noexcept;

    void initialize() const;
    void set(bool enabled) const;

    [[nodiscard]] gpio_num_t pin() const noexcept;

private:
    GpioOutput output_;
};

}  // namespace nimbus::driver
