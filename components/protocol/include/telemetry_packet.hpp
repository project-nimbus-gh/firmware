#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace nimbus::protocol {

constexpr std::size_t kPacketSize = 18;
constexpr uint8_t kPacketStart = 0x02;
constexpr uint8_t kPacketEnd = 0x03;

using PacketBytes = std::array<uint8_t, kPacketSize>;

struct SensorPacket {
    uint8_t type;
    float temperature_c;
    float humidity_percent;
    uint16_t air_pressure_hpa;
    uint32_t serial;
};

[[nodiscard]] PacketBytes encodePacket(const SensorPacket &packet);
[[nodiscard]] std::optional<SensorPacket> decodePacket(const PacketBytes &packet);
[[nodiscard]] bool isValidPacketFrame(const PacketBytes &packet) noexcept;

}  // namespace nimbus::protocol
