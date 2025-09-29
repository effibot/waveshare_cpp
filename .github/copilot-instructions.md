# Copilot Instructions for Waveshare USB-CAN-A C++ Library

## Project Overview
This library provides a C++17+ interface for the Waveshare USB-CAN-A device, supporting CAN message transmission, configuration, and error handling. It is organized around protocol frame types and leverages CRTP for extensibility and performance.

## Architecture & Key Files
- **Frame System (CRTP-based):**
  - `include/base_frame.hpp`: Abstract base for all frame types.
  - `include/fixed_frame.hpp`, `include/variable_frame.hpp`: Implement fixed/variable CAN frames.
  - `include/config_frame.hpp`: Implements configuration frames (inherits from `fixed_frame`).
- **Device Interface:**
  - `include/usb_can.hpp`: Main class for USB-CAN-A device communication (open/close, send/receive, config).
- **Utilities & Protocol:**
  - `include/common.hpp`: Protocol constants, error codes, helpers.
  - `include/checksum_utils.hpp`: Checksum calculation for frames.

## Protocol Details
- **Frame Types:**
  - Fixed frames: 20 bytes, padded data, checksum covers `TYPE` to last `RESERVED`.
  - Variable frames: Size varies, type field encodes frame type, format, DLC.
- **Bitwise Operations:**
  - Variable frame type is constructed using bitwise ops (see README for example).

## Developer Workflows
- **Build:**
  - Requires C++17+, `libusb-1.0`.
  - Typical build: `g++ -std=c++17 -Iinclude -o main main.cpp src/*.cpp -lusb-1.0`
- **Test:**
  - Test entry: `test_main_equivalent.cpp` (mirrors main logic for test/dev).
- **Debug:**
  - Use verbose logging in device interface and frame classes for protocol debugging.

## Project Conventions
- **Frame Construction:**
  - Always use provided frame classes for protocol compliance.
  - Checksum must be calculated as described in README.
- **Error Handling:**
  - Use error codes from `common.hpp`.
- **Data Padding:**
  - Fixed frames: pad data to 8 bytes if DLC < 8.
  - Variable frames: DLC matches data length.

## Integration Points
- **External Dependency:**
  - `libusb-1.0` for USB communication.
- **Cross-Component Communication:**
  - Frame classes interact via inheritance and composition; device interface uses frames for all I/O.

## Examples
- Constructing a variable frame type:
  ```cpp
  uint8_t type = 0xC0 | (frame_type << 5) | (frame_format << 4) | (dlc & 0x0F);
  ```
- Building and sending a CAN message:
  ```cpp
  FixedFrame frame(...); // see fixed_frame.hpp
  UsbCan device;
  device.open();
  device.send(frame);
  ```

## References
- See `README.md` for protocol and frame details.
- See `include/` for all frame and device interface headers.

---
*Update this file as project conventions evolve. Feedback welcome!*
