#include "telemetry_packet.hpp"

#include <cmath>

namespace nimbus::telemetry {
namespace {

typedef struct {
    uint64_t low;
    uint64_t high;
} uint128_parts;

uint32_t popcount128(uint128_parts value)
{
    return __builtin_popcountll(value.low) + __builtin_popcountll(value.high);
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

    // Split 128-bit logic into two 64-bit halves
    uint64_t low = 0;
    uint64_t high = 0;

    // Bits 0-63 (Handling type, temperature, humidity, pressure)
    low |= static_cast<uint64_t>(packet.type & 0x0F) << 0;
    low |= static_cast<uint64_t>(temperature_scaled & 0x7FFF) << 4;
    low |= static_cast<uint64_t>(humidity_scaled & 0x03FF) << 19;
    low |= static_cast<uint64_t>(pressure_scaled & 0x03FF) << 29;

    // Bits 95-126 (Serial starts at bit 95, which is bit 31 of the 'high' half)
    high |= static_cast<uint64_t>(packet.serial) << 31;

    // Calculate parity using your new struct-based function
    uint128_parts raw_parts = { .low = low, .high = high };
    if ((popcount128(raw_parts) % 2U) != 0U) {
        high |= static_cast<uint64_t>(1) << 63; // Bit 127 total
    }

    std::array<uint8_t, kPacketSize> encoded{};
    encoded[0] = kPacketStart;
    
    // Manually serialize bytes from high to low
    for (std::size_t index = 0; index < 8; ++index) {
        encoded[index + 1] = static_cast<uint8_t>(high >> (56 - (index * 8)));
    }
    for (std::size_t index = 0; index < 8; ++index) {
        encoded[index + 9] = static_cast<uint8_t>(low >> (56 - (index * 8)));
    }
    
    encoded[17] = kPacketEnd;
    return encoded;
}

}  // namespace nimbus::telemetry
