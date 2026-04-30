# Nimbus Firmware

ESP32 firmware for the Nimbus device using ESP-IDF and CMake.

## Prerequisites

- ESP-IDF v5+ installed and exported
- ESP32 toolchain (installed through ESP-IDF tools)

## Getting Started

From this folder:

```bash
idf.py set-target esp32
idf.py build
```

## Flash and Monitor

```bash
idf.py -p <PORT> flash monitor
```

Example port on macOS: `/dev/cu.usbserial-xxxx`

## Notes

- Main firmware sources are in `main/`.
- Build artifacts are generated in `build/`.
- Current demo behavior includes status LED blinking and synthetic telemetry logging.

## Project Structure

- `main/app_main.cpp`: Firmware entry point
- `main/demo_telemetry.cpp`: Demo telemetry generation
- `main/telemetry_packet.cpp`: Telemetry packet encoding
- `main/status_led.cpp`: Status LED control
- `CMakeLists.txt`: Top-level ESP-IDF project definition
