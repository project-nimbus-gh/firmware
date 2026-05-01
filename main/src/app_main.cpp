#include <array>
#include <cstddef>
#include <cstdio>
#include <vector>

#include "demo_telemetry.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "connectivity.hpp"
#include "status_led.hpp"
#include "telemetry_packet.hpp"

namespace {

constexpr const char *kTag = "nimbus_firmware";

void logPacket(const nimbus::protocol::PacketBytes &packet)
{
    char hex_buffer[nimbus::protocol::kPacketSize * 3 + 1];
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
    nimbus::connect::ConnectivityStack connectivity;
    connectivity.initialize();

    nimbus::driver::StatusLed status_led;
    status_led.initialize();

    nimbus::demo::DemoTelemetrySource telemetry_source;

    ESP_LOGI(kTag, "Status LED is on GPIO %d", status_led.pin());
    ESP_LOGI(kTag, "Connectivity stack initialized for Wi-Fi and Bluetooth examples");

    std::vector<nimbus::connect::WifiNetwork> networks = connectivity.getWifi().scanAvailableNetworks();

    for (const auto &network : networks) {
        ESP_LOGI(kTag, "Found Wi-Fi network: SSID='%s', BSSID=%s, RSSI=%d dBm, Channel=%d, AuthMode=%d",
                 network.ssid.c_str(), network.bssid.c_str(), network.rssi, network.channel, network.auth_mode);
    }

    bool led_enabled = false;

    while (true) {
        const auto packet = telemetry_source.next();
        const auto encoded_packet = nimbus::protocol::encodePacket(packet);

        led_enabled = !led_enabled;
        status_led.set(led_enabled);
        logPacket(encoded_packet);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
