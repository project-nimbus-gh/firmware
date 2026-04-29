#pragma once

#include "driver/gpio.h"

namespace nimbus::hardware {

class StatusLed {
public:
    explicit StatusLed(gpio_num_t pin = GPIO_NUM_2);

    void initialize() const;
    void set(bool enabled) const;

private:
    gpio_num_t pin_;
};

}  // namespace nimbus::hardware
