#pragma once

#include "telemetry_packet.hpp"

namespace nimbus::demo {

class DemoTelemetrySource {
public:
    DemoTelemetrySource();

    telemetry::SensorPacket next();

private:
    uint32_t serial_;
};

}  // namespace nimbus::demo
