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
- The firmware now boots as a BLE provisioning service named `Nimbus-Setup`.
- Bluetooth messages use JSON over a custom GATT service.
- The default provisioning password is `default` and must be changed after the first successful Bluetooth login.

## Project Structure

- `components/protocol/`: Packet building, encoding, decoding, and frame validation
- `components/driver/`: Direct GPIO hardware wrappers for LEDs and outputs
- `components/connect/`: Wi-Fi and Bluetooth initialization helpers
- `components/provisioning/`: BLE provisioning, Wi-Fi storage, and remote management
- `main/app_main.cpp`: Firmware entry point for provisioning mode
- `CMakeLists.txt`: Top-level ESP-IDF project definition
