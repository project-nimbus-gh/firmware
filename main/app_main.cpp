#include <cstddef>
#include <array>
#include <cstdio>

#include "demo_telemetry.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "status_led.hpp"
#include "telemetry_packet.hpp"

namespace {

constexpr const char *kTag = "nimbus_firmware";

void logPacket(const std::array<uint8_t, nimbus::telemetry::kPacketSize> &packet)
{
    char hex_buffer[nimbus::telemetry::kPacketSize * 3 + 1];
    std::size_t offset = 0;

    for (std::size_t index = 0; index < packet.size(); ++index) {
        offset += static_cast<std::size_t>(std::snprintf(
            &hex_buffer[offset], sizeof(hex_buffer) - offset,
            index == 0 ? "%02X" : " %02X", packet[index]));
    }

    hex_buffer[sizeof(hex_buffer) - 1] = '\0';
    ESP_LOGI(kTag, "Telemetry frame: %s", hex_buffer);
}

}  // namespace

extern "C" void app_main(void)
{
    nimbus::hardware::StatusLed status_led;
    status_led.initialize();

    nimbus::demo::DemoTelemetrySource telemetry_source;

    ESP_LOGI(kTag, "Status LED is on GPIO %d", GPIO_NUM_2);

    bool led_enabled = false;

    while (true) {
        const auto packet = telemetry_source.next();
        const auto encoded_packet = nimbus::telemetry::encodePacket(packet);

        led_enabled = !led_enabled;
        status_led.set(led_enabled);
        logPacket(encoded_packet);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
