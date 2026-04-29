#include "telemetry_packet.hpp"

#include <cmath>

namespace nimbus::telemetry {
namespace {

uint32_t popcount128(__uint128_t value)
{
    return static_cast<uint32_t>(__builtin_popcountll(static_cast<uint64_t>(value))) +
           static_cast<uint32_t>(__builtin_popcountll(static_cast<uint64_t>(value >> 64)));
}

float clampFloat(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

uint16_t clampTemperatureScaled(float temperature_c)
{
    const float clamped = clampFloat(temperature_c, -150.0f, 150.0f);
    return static_cast<uint16_t>(lroundf((clamped + 150.0f) * 100.0f));
}

uint16_t clampHumidityScaled(float humidity_percent)
{
    const float clamped = clampFloat(humidity_percent, 0.0f, 100.0f);
    return static_cast<uint16_t>(lroundf(clamped * 10.0f));
}

uint16_t clampPressureScaled(uint16_t air_pressure_hpa)
{
    if (air_pressure_hpa < 300U) {
        air_pressure_hpa = 300U;
    } else if (air_pressure_hpa > 1100U) {
        air_pressure_hpa = 1100U;
    }
    return static_cast<uint16_t>(air_pressure_hpa - 300U);
}

}  // namespace

std::array<uint8_t, kPacketSize> encodePacket(const SensorPacket &packet)
{
    const uint16_t temperature_scaled = clampTemperatureScaled(packet.temperature_c);
    const uint16_t humidity_scaled = clampHumidityScaled(packet.humidity_percent);
    const uint16_t pressure_scaled = clampPressureScaled(packet.air_pressure_hpa);

    __uint128_t raw = 0;
    raw |= static_cast<__uint128_t>(packet.type & 0x0F) << 0;
    raw |= static_cast<__uint128_t>(temperature_scaled & 0x7FFF) << 4;
    raw |= static_cast<__uint128_t>(humidity_scaled & 0x03FF) << 19;
    raw |= static_cast<__uint128_t>(pressure_scaled & 0x03FF) << 29;
    raw |= static_cast<__uint128_t>(packet.serial) << 95;

    if ((popcount128(raw) % 2U) != 0U) {
        raw |= static_cast<__uint128_t>(1) << 127;
    }

    std::array<uint8_t, kPacketSize> encoded{};
    encoded[0] = kPacketStart;
    for (std::size_t index = 0; index < 16; ++index) {
        encoded[index + 1] = static_cast<uint8_t>(raw >> (120 - (index * 8)));
    }
    encoded[17] = kPacketEnd;
    return encoded;
}

}  // namespace nimbus::telemetry
