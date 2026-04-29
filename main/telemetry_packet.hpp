#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nimbus::telemetry {

constexpr std::size_t kPacketSize = 18;
constexpr uint8_t kPacketStart = 0x02;
constexpr uint8_t kPacketEnd = 0x03;

struct SensorPacket {
    uint8_t type;
    float temperature_c;
    float humidity_percent;
    uint16_t air_pressure_hpa;
    uint32_t serial;
};

std::array<uint8_t, kPacketSize> encodePacket(const SensorPacket &packet);

}  // namespace nimbus::telemetry
