#include "demo_telemetry.hpp"

namespace nimbus::demo {

DemoTelemetrySource::DemoTelemetrySource() : serial_(1) {}

telemetry::SensorPacket DemoTelemetrySource::next()
{
    const telemetry::SensorPacket packet{
        1,
        21.50f + static_cast<float>(serial_ % 10U) * 0.25f,
        45.0f + static_cast<float>(serial_ % 15U) * 0.5f,
        static_cast<uint16_t>(1008U + (serial_ % 5U)),
        serial_,
    };

    ++serial_;
    return packet;
}

}  // namespace nimbus::demo
