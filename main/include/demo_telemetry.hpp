#pragma once

#include "telemetry_packet.hpp"

namespace nimbus::demo {

class DemoTelemetrySource {
public:
    [[nodiscard]] protocol::SensorPacket next();

private:
    uint32_t serial_ {1};
};

}  // namespace nimbus::demo
