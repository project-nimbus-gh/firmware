#include "status_led.hpp"

namespace nimbus::driver {

StatusLed::StatusLed(gpio_num_t pin) noexcept : output_(pin) {}

void StatusLed::initialize() const
{
    output_.initialize();
}

void StatusLed::set(bool enabled) const
{
    output_.set(enabled);
}

gpio_num_t StatusLed::pin() const noexcept
{
    return output_.pin();
}

}  // namespace nimbus::driver
