#include "telemetry_packet.hpp"

#include <cmath>

namespace nimbus::protocol {
namespace {

struct PackedBits {
    uint64_t low;
    uint64_t high;
};

[[nodiscard]] uint32_t popcount128(const PackedBits &value)
{
    return static_cast<uint32_t>(__builtin_popcountll(value.low) + __builtin_popcountll(value.high));
}

[[nodiscard]] float clampFloat(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

[[nodiscard]] uint16_t clampTemperatureScaled(float temperature_c)
{
    const float clamped = clampFloat(temperature_c, -150.0f, 150.0f);
    return static_cast<uint16_t>(std::lround((clamped + 150.0f) * 100.0f));
}

[[nodiscard]] uint16_t clampHumidityScaled(float humidity_percent)
{
    const float clamped = clampFloat(humidity_percent, 0.0f, 100.0f);
    return static_cast<uint16_t>(std::lround(clamped * 10.0f));
}

[[nodiscard]] uint16_t clampPressureScaled(uint16_t air_pressure_hpa)
{
    if (air_pressure_hpa < 300U) {
        air_pressure_hpa = 300U;
    } else if (air_pressure_hpa > 1100U) {
        air_pressure_hpa = 1100U;
    }

    return static_cast<uint16_t>(air_pressure_hpa - 300U);
}

[[nodiscard]] PackedBits unpackBits(const PacketBytes &packet)
{
    PackedBits bits{};

    for (std::size_t index = 0; index < 8; ++index) {
        bits.high = (bits.high << 8) | packet[index + 1];
    }

    for (std::size_t index = 0; index < 8; ++index) {
        bits.low = (bits.low << 8) | packet[index + 9];
    }

    return bits;
}

[[nodiscard]] PacketBytes packBits(const PackedBits &bits)
{
    PacketBytes packet{};
    packet[0] = kPacketStart;

    for (std::size_t index = 0; index < 8; ++index) {
        packet[index + 1] = static_cast<uint8_t>(bits.high >> (56U - (index * 8U)));
    }

    for (std::size_t index = 0; index < 8; ++index) {
        packet[index + 9] = static_cast<uint8_t>(bits.low >> (56U - (index * 8U)));
    }

    packet[17] = kPacketEnd;
    return packet;
}

[[nodiscard]] bool hasEvenParity(const PackedBits &bits) noexcept
{
    return (popcount128(bits) % 2U) == 0U;
}

}  // namespace

PacketBytes encodePacket(const SensorPacket &packet)
{
    const uint16_t temperature_scaled = clampTemperatureScaled(packet.temperature_c);
    const uint16_t humidity_scaled = clampHumidityScaled(packet.humidity_percent);
    const uint16_t pressure_scaled = clampPressureScaled(packet.air_pressure_hpa);

    PackedBits bits{};
    bits.low |= static_cast<uint64_t>(packet.type & 0x0F) << 0;
    bits.low |= static_cast<uint64_t>(temperature_scaled & 0x7FFFU) << 4;
    bits.low |= static_cast<uint64_t>(humidity_scaled & 0x03FFU) << 19;
    bits.low |= static_cast<uint64_t>(pressure_scaled & 0x03FFU) << 29;
    bits.high |= static_cast<uint64_t>(packet.serial) << 31;

    if (!hasEvenParity(bits)) {
        bits.high |= static_cast<uint64_t>(1U) << 63;
    }

    return packBits(bits);
}

bool isValidPacketFrame(const PacketBytes &packet) noexcept
{
    if (packet.front() != kPacketStart || packet.back() != kPacketEnd) {
        return false;
    }

    return hasEvenParity(unpackBits(packet));
}

std::optional<SensorPacket> decodePacket(const PacketBytes &packet)
{
    if (!isValidPacketFrame(packet)) {
        return std::nullopt;
    }

    const PackedBits bits = unpackBits(packet);

    SensorPacket decoded{};
    decoded.type = static_cast<uint8_t>(bits.low & 0x0FU);
    decoded.temperature_c = (static_cast<float>((bits.low >> 4) & 0x7FFFU) / 100.0f) - 150.0f;
    decoded.humidity_percent = static_cast<float>((bits.low >> 19) & 0x03FFU) / 10.0f;
    decoded.air_pressure_hpa = static_cast<uint16_t>(((bits.low >> 29) & 0x03FFU) + 300U);
    decoded.serial = static_cast<uint32_t>((bits.high >> 31) & 0xFFFFFFFFULL);

    return decoded;
}

}  // namespace nimbus::protocol
