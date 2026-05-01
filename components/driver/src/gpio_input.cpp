#include "gpio_input.hpp"

#include "esp_err.h"

namespace nimbus::driver {

GpioInput::GpioInput(gpio_num_t pin, bool pull_up, bool pull_down) noexcept
    : pin_(pin), pull_up_(pull_up), pull_down_(pull_down) {}

void GpioInput::initialize() const
{
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << pin_;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = pull_up_ ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    config.pull_down_en = pull_down_ ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&config));
}

int GpioInput::read() const
{
    return gpio_get_level(pin_);
}

gpio_num_t GpioInput::pin() const noexcept
{
    return pin_;
}

}  // namespace nimbus::driver
