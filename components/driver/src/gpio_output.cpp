#include "gpio_output.hpp"

#include "esp_err.h"

namespace nimbus::driver {

GpioOutput::GpioOutput(gpio_num_t pin) noexcept : pin_(pin) {}

void GpioOutput::initialize() const
{
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << pin_;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_ERROR_CHECK(gpio_set_level(pin_, 0));
}

void GpioOutput::set(bool enabled) const
{
    ESP_ERROR_CHECK(gpio_set_level(pin_, enabled ? 1 : 0));
}

gpio_num_t GpioOutput::pin() const noexcept
{
    return pin_;
}

}  // namespace nimbus::driver
