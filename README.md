# Waveshare USB-CAN-A C++ Library
This is a C++ library for interfacing with the Waveshare USB-CAN-A device. The library provides functions to initialize the device, send and receive CAN messages using CANOpen standards, and configure USB adapter settings.
## Features
- Initialize and configure the USB-CAN-A device
- Send and receive CAN messages with both fixed and variable length frames
- Support for both standard (11-bit - CAN 2.0A) and extended (29-bit - CAN 2.0B) CAN IDs
- Error handling and status reporting
## Requirements
- C++17 or later
- libusb-1.0


## Implementation Details

- The `FRAME_TYPE` field used in the [protocol User Manual](doc/Waveshare_USB-CAN-A_Users_Manual_EN.pdf) is represented in the library as an enum class `CAN_VERS`, to avoid confusion with the `Type` field which is represented as an enum class `Type` to differentiate between fixed, variable and config frames.

## Protocol Details
The Waveshare USB-CAN-A device uses a custom protocol for communication. The protocol consists of 3 types of frames where each field is represented by a byte unless otherwise specified. The protocol allows to use both CAN 2.0A and CAN 2.0B frames, supporting ID lengths of 11 and 29 bits respectively, depending on the value of the `CAN_VERS` field. 
> The constants for the protocol are defined in `/include/enums/protocol.hpp`.

The most important values are the following:
- `START`: `0xAA`, indicates the start of a frame.
    > indicated as `S` (Start Of Transmission) in the packet diagrams.
- `HEADER`: `0x55`, indicates the header of a frame (used only in fixed and config frames).
    > indicated as `H` (Header) in the packet diagrams.
- `TYPE`: `0x01`,`0x02`,`0x12` and a custom value for the variable frame (see below). Indicates the type of the frame, respectivly for fixed data frame, config frame for fixed data frame, config frame for variable data frame and variable data frame.
    > indicated as `T` (Type) in the packet diagrams.
- `CANVersion`: `0x01`, `0x02`, `0`, `1`. Indicates if standard or extended CAN ID is used, respectively for fixed and variable frames. 
    > indicated as `E` (IsExtended) in the packet diagrams.
- `FRAME_FORMAT`: `0x01`, `0x02`, `0`, `1`. Indicates if the frame is a data frame or a remote frame, respectively for fixed and variable frames.
    > indicated as `F` (Format) in the packet diagrams.
- `CANBaud`: `0x01` to `0x0C`. Indicates the baud rate for CAN communication, with values ranging from 1 Mbps to 5 kbps.
    > indicated as `B` (Baud Rate) in the packet diagrams.
- `SerialBaud`: `9600` to `2000000`. Indicates the baud rate for serial communication, with values ranging from 9600 bps to 2 Mbps. They are common values used in serial communication.    
- `CANMode`: `0x00`, `0x01`, `0x02`, `0x03`. Indicates the mode of operation for the CAN controller, respectively normal mode, loopback mode, silent mode (listen-only) and silent loopback mode.
    > indicated as `M` (Operating Mode) in the packet diagrams.
- `RTX`: `0x00`, `0x01`. Indicates if the CAN controller should automatically retransmit messages that fail to be acknowledged (`0x00`) or not (`0x01`).
    > indicated as `RTX` (Retransmission) in the packet diagrams.
- `RESERVED`: Reserved bytes for future use, should be set to `0x00`.
    > indicated as `R` (Reserved) in the packet diagrams.
- `END`: `0x55`, indicates the end of a frame (used only in variable frames).
    > indicated as `END` (End Of Transmission) in the packet diagrams.

# Frames Structure:

- **Fixed Frames**: These frames have a fixed size of 20 bytes and are used both for set the configuration of the USB device and for sending/receiving CAN messages with up to 8 bytes of data. The structure of a fixed **data** frame is as follows:
![](doc//diagrams/waveshare_fixed_data_packet.md)


- The structure of a fixed **config** frame is as follows:
![](doc//diagrams/waveshare_config_packet.md)

> For both types of fixed frames, the checksum is calculated as the sum of all bytes from `TYPE` to the last `RESERVED` byte, taking only the least significant byte of the result.


- **Variable Frames**: These frames have a variable size and are used for sending/receiving CAN messages without the need to pad the data to 8 bytes. 
    - The `ID` field can be either of 2 or 4 bytes, depending on the value of bit 5 of the `TYPE` field. 
    - The `DATA` field can be from 0 to 8 bytes, depending on the value of bits 3-0 of the `TYPE` field. The `END` field is always `0x55`. 

    The structure of the frame is as follows:
![](doc/diagrams/waveshare_variable_data_packet.md)

    In this case, the `TYPE` field is **not** defined as a constant but as a byte with the following bit structure (**This field should be read from right to left**):
![](doc/diagrams/waveshare_variable_frame_type.md)

    Where `Type` is a constant value of `0xC0` (binary `11000000`), the `IsExtended` bit indicates if the ID is 11 or 29 bits long, the `Format` bit indicates if the frame is a data or remote frame, and the `DLC` (Data Length Code) indicates the number of data bytes in the frame (from 0 to 8).

    > Using a `uint8_t` to represent the `Type` byte allows to easily extract the individual fields using bitwise operations.

    For example, starting with `0xC0` to sets the two most significant bits to `11`, one can do the following:
```cpp
uint8_t type = 0xC0 | (frame_type << 5) | (frame_format << 4) | (dlc & 0x0F);
```

## Note on ID construction
The ID used in the protocol message can be either 11 or 29 bits long, depending on the value of the `CAN_VERS` field. The ID is represented as a 32-bit unsigned integer (`uint32_t`) in the library, but only the lower 11 or 29 bits are used.
> The ID is stored in the protocol message in little endian format, meaning that the least significant byte is stored first. For example, a Standard ID of `0x123` would be stored as: `[0x23][0x01]`



## Note on DLC and Data Padding

The DLC (Data Length Code) field indicates the number of data bytes in the CAN message. It tells how many bytes of `DATA` contains a valid information that should be processed. 

- For fixed frames, the `DATA` field is statically allocated to 8 bytes, so if the DLC is less than 8, the remaining bytes should be padded with zeros. This means that the `DLC` defines how many bytes of the `DATA` field are actually used, while the size of the `DATA` field itself is always 8 bytes.
- For variable frames, the `DATA` field is the one that defines the size of the frame itself, so it's the length of the `DATA` field which defines the DLC.

> This is practically relevant only when you are sending and receiving CAN messages with the USB-CAN-A adapter. When sending a CAN message, another node of the CAN newtork will only see the effective CAN message, not the packet that is used in this library. When receiving a CAN message, the library will parse the serial data and build the appropriate frame structure, so the user will only see the effective CAN message.

## Note on Filtering and Masking

When configuring the acceptance filter and mask, it's important to understand how they work together to determine which CAN messages are accepted by the USB-CAN-A device. 
> It's not necessary to set a filter and/or a mask, but if you do, the following rules apply:
    - The sender's frame ID should match the receiver's filter ID.
    - Both filter and mask are hexadecimal values.
    - The lower 11 bits of filter ID and mask ID are valid in a standard frame (range: `0x00000000~0x000007ff`), and the lower 29 bits of filter ID and mask ID in the extended frame are valid (range `0x00000000~0x1fffffff`).


### Bibliography
- [Waveshare USB-CAN-A User Manual](https://www.waveshare.com/wiki/USB-CAN-A#Software_Settings)
- [Waveshare USB-CAN-A Secondary Development](https://www.waveshare.com/wiki/Secondary_Development_Serial_Conversion_Definition_of_CAN_Protocol)
- [Waveshare USB-CAN-A Driver for Linux](https://files.waveshare.com/wiki/USB-CAN-A/Demo/USB-CAN-A.zip)
- [CAN Specification 2.0](https://www.can-cia.org/can-knowledge/can-cc)

---

## Migration to Exception-Based Error Handling

### Overview
The library currently uses `Result<T>` pattern for error handling. We're migrating to standard C++ exceptions for:
- **Simpler API**: No need to check `.success()/.fail()` on every call
- **Less code**: Remove Result wrapper and error chaining boilerplate  
- **Standard C++ idioms**: More familiar to C++ developers
- **Better stack traces**: Native exception unwinding
- **Zero-cost in happy path**: Exceptions are faster than Result checks when no error occurs

### Migration Strategy
1. **Create exception hierarchy** based on existing `Status` enum
2. **Bottom-up refactoring**: Helpers → Frames → Adapter → Bridge
3. **Update tests concurrently** with each component
4. **Remove Result<T> infrastructure** at the end

### Quick Reference: Exception Types

| Exception Type | Used For | Replaces Status Codes |
|---------------|----------|---------------------|
| `ProtocolException` | Frame validation, protocol errors | `WBAD_*` codes |
| `DeviceException` | I/O errors, device issues | `DNOT_*`, `DREAD_*`, `DWRITE_*` codes |
| `TimeoutException` | Read/write timeouts | `WTIMEOUT` |
| `CANException` | CAN bus protocol errors | `CAN_*` codes |

### Exception Hierarchy Design

```cpp
namespace waveshare {
    // Base exception class
    class WaveshareException : public std::runtime_error {
        Status status_;  // Original error code
        std::string context_;  // Function/operation context
    public:
        WaveshareException(Status status, const std::string& context);
        Status status() const noexcept { return status_; }
        const char* what() const noexcept override;
    };

    // Protocol/frame validation errors (WBAD_*)
    class ProtocolException : public WaveshareException { };
    
    // Device I/O errors (DNOT_*, DREAD_*, DWRITE_*)
    class DeviceException : public WaveshareException { };
    
    // Timeout errors (WTIMEOUT)
    class TimeoutException : public WaveshareException { };
    
    // CAN bus protocol errors (CAN_*)
    class CANException : public WaveshareException { };
}
```

### Migration Checklist

#### Phase 0: Create Exception Infrastructure ✅
- [x] **0.1 Create exception header**
  - [x] Create `include/exception/waveshare_exception.hpp`
  - [x] Define base `WaveshareException` class with `Status` member
  - [x] Define derived classes: `ProtocolException`, `DeviceException`, `TimeoutException`, `CANException`
  - [x] Implement constructors that take `Status` and context string
  - [x] Implement `what()` to format: `"[Status] context: message"`

- [x] **0.2 Create exception factory helpers**
  - [x] Add static factory methods to base class:
    - [x] `static void throw_if_error(Status status, const std::string& context)`
    - [x] `static void throw_error(Status status, const std::string& context)` 
  - [x] Map Status codes to appropriate exception types

- [x] **0.3 Update error.hpp**
  - [x] Keep `Status` enum (still useful for categorization)
  - [x] Keep `USBCANErrorCategory` (useful for std::error_code interop if needed)

- [x] **0.4 Create exception tests**
  - [x] Test all exception types can be thrown and caught
  - [x] Test factory functions route to correct exception types
  - [x] Test polymorphic catching (all exceptions catchable as WaveshareException)
  - [x] Verify error messages contain status and context
  - [x] All tests passing (76/76)

#### Phase 1: Update Serialization Helpers ⬜
- [ ] **1.1 Update ChecksumHelper**
  - [ ] Change `validate()` from `Result<void>` to `void` (throws on error)
  - [ ] Throw `ProtocolException` on checksum mismatch
  - [ ] Remove Result includes

- [ ] **1.2 Update VarTypeHelper**
  - [ ] Change return types from `Result<T>` to `T`
  - [ ] Throw `ProtocolException` on invalid type byte
  - [ ] Update tests to use `REQUIRE_THROWS_AS()`

#### Phase 2: Update Frame Interfaces ⬜
- [ ] **2.1 Update CoreInterface**
  - [ ] Change `deserialize()` from `Result<void>` to `void`
  - [ ] Change `impl_deserialize()` from `Result<void>` to `void`
  - [ ] Remove Result includes
  - [ ] Update all derived implementations

- [ ] **2.2 Update DataInterface**
  - [ ] Change all setters to throw on invalid input (instead of returning Result)
  - [ ] Validate in setters: throw `ProtocolException` on bad ID, DLC, etc.
  - [ ] Update tests

- [ ] **2.3 Update ConfigInterface**
  - [ ] Same pattern as DataInterface
  - [ ] Throw on invalid baud rates, modes, filters

#### Phase 3: Update Frame Implementations ⬜
- [ ] **3.1 Update FixedFrame**
  - [ ] Change `impl_deserialize()` to `void` (throws on error)
  - [ ] Replace all `return Result<void>::error(...)` with `throw ProtocolException(...)`
  - [ ] Replace all `if (res.fail()) return res;` with try-catch or let exceptions propagate
  - [ ] Update `src/fixed_frame.cpp`
  - [ ] Update tests: use `REQUIRE_THROWS_AS()` for invalid frames

- [ ] **3.2 Update VariableFrame**
  - [ ] Same pattern as FixedFrame
  - [ ] Update `src/variable_frame.cpp`
  - [ ] Update tests

- [ ] **3.3 Update ConfigFrame**
  - [ ] Same pattern
  - [ ] Update `src/config_frame.cpp`
  - [ ] Update tests

#### Phase 4: Update FrameBuilder ⬜
- [ ] **4.1 Update builder validation**
  - [ ] Change `build()` from `Result<Frame>` to `Frame` (throws on validation error)
  - [ ] Throw `ProtocolException` for missing required fields
  - [ ] Throw `ProtocolException` for invalid combinations
  - [ ] Update tests

#### Phase 5: Update USBAdapter ⬜
- [ ] **5.1 Update low-level I/O methods**
  - [ ] Change `write_bytes()` from `Result<int>` to `int` (throws on error)
  - [ ] Change `read_bytes()` from `Result<int>` to `int`
  - [ ] Change `read_exact()` from `Result<void>` to `void`
  - [ ] Throw `DeviceException` on I/O errors
  - [ ] Throw `TimeoutException` on timeout

- [ ] **5.2 Update port management**
  - [ ] Change `open_port()` from `Result<void>` to `void`
  - [ ] Change `configure_port()` from `Result<void>` to `void`
  - [ ] Change `close_port()` from `Result<void>` to `void`
  - [ ] Throw `DeviceException` on errors

- [ ] **5.3 Update frame-level API**
  - [ ] Change `send_frame()` from `Result<void>` to `void`
  - [ ] Change `receive_fixed_frame()` from `Result<FixedFrame>` to `FixedFrame`
  - [ ] Change `receive_variable_frame()` from `Result<VariableFrame>` to `VariableFrame`
  - [ ] Update `src/usb_adapter.cpp`

- [ ] **5.4 Update constructor**
  - [ ] Constructor already throws on error (good!)
  - [ ] Verify error messages are clear

- [ ] **5.5 Update tests**
  - [ ] Replace all `REQUIRE(result.success())` with direct calls
  - [ ] Replace all `REQUIRE(result.fail())` with `REQUIRE_THROWS_AS()`
  - [ ] Update `test/test_usb_adapter.cpp`

#### Phase 6: Update Scripts ⬜
- [ ] **6.1 Update wave_reader.cpp**
  - [ ] Replace Result checking with try-catch blocks
  - [ ] Catch exceptions at thread loop boundary
  - [ ] Log exception messages using `e.what()`

- [ ] **6.2 Update wave_writer.cpp**
  - [ ] Same pattern as wave_reader
  - [ ] Cleaner error handling in main loop

- [ ] **6.3 Update script_utils.hpp**
  - [ ] Update helper functions to throw instead of returning Result
  - [ ] Update parse_arguments() and initialize_adapter()

#### Phase 7: Remove Result Infrastructure ⬜
- [ ] **7.1 Remove Result header**
  - [ ] Delete `include/template/result.hpp`
  - [ ] Verify no remaining includes

- [ ] **7.2 Update documentation**
  - [ ] Remove Result<T> from `.github/copilot-instructions.md`
  - [ ] Update to document exception hierarchy
  - [ ] Update error handling examples

- [ ] **7.3 Update test reference**
  - [ ] Remove Result<T> section from `test/CATCH2_REFERENCE.md`
  - [ ] Add exception testing section

#### Phase 8: Update Build System ⬜
- [ ] **8.1 Clean rebuild**
  - [ ] Run `cmake --build build --target clean`
  - [ ] Reconfigure: `cmake -S . -B build`
  - [ ] Build: `cmake --build build`
  - [ ] Verify no compilation errors

- [ ] **8.2 Run full test suite**
  - [ ] `cd build && ctest --output-on-failure`
  - [ ] Verify all 68+ tests pass
  - [ ] Fix any remaining issues

#### Phase 9: Benefits for SocketCAN Bridge ⬜
After migration, the SocketCAN bridge implementation will be simpler:

**Before (with Result<T>):**
```cpp
void usb_to_socketcan_loop() {
    while (running_) {
        auto frame_result = adapter_->receive_variable_frame(timeout);
        if (frame_result.fail()) {
            if (frame_result.error() == Status::WTIMEOUT) continue;
            stats_.usb_rx_errors++;
            log_error(frame_result.describe());
            continue;
        }
        auto cf_result = SocketCANHelper::to_socketcan(frame_result.value());
        if (cf_result.fail()) {
            stats_.conversion_errors++;
            continue;
        }
        // ... write to socket
    }
}
```

**After (with exceptions):**
```cpp
void usb_to_socketcan_loop() {
    while (running_) {
        try {
            auto frame = adapter_->receive_variable_frame(timeout);
            auto cf = SocketCANHelper::to_socketcan(frame);
            // ... write to socket
            stats_.usb_rx_frames++;
        } catch (const TimeoutException&) {
            continue;  // Expected, just retry
        } catch (const WaveshareException& e) {
            stats_.usb_rx_errors++;
            log_error(e.what());
        }
    }
}
```

**Benefits:**
- 40% less code in loops
- No Result wrapper overhead
- Clear separation of timeout (expected) vs errors
- Natural exception propagation to thread boundary
- Easier to add new error types

---

## SocketCAN Bridge Implementation Checklist

### Phase 1: Foundation - SocketCAN Conversion Helpers ⬜
**Goal**: Add bidirectional frame conversion between Waveshare VariableFrame and Linux `struct can_frame`

**Note**: After exception migration, these will throw instead of returning Result<T>

- [ ] **1.1 Create helper header**
  - [ ] Create `include/interface/socketcan_helpers.hpp`
  - [ ] Add include guards and namespace `waveshare`
  - [ ] Include dependencies: `<linux/can.h>`, `<linux/can/raw.h>`
  - [ ] Include `include/frame/variable_frame.hpp` and exception header

- [ ] **1.2 Declare SocketCANHelper class**
  - [ ] Create static class `SocketCANHelper` (no instances, all static methods)
  - [ ] Declare `static can_frame to_socketcan(const VariableFrame& frame)` (throws on error)
  - [ ] Declare `static VariableFrame from_socketcan(const can_frame& cf)` (throws on error)
  - [ ] Add documentation comments explaining conversion rules

- [ ] **1.3 Implement to_socketcan() conversion**
  - [ ] Create `src/socketcan_helpers.cpp`
  - [ ] Extract CAN ID from VariableFrame (handle standard vs extended)
  - [ ] Set `can_frame.can_id` with appropriate flags (`CAN_EFF_FLAG` for extended, `CAN_RTR_FLAG` for remote)
  - [ ] Copy DLC to `can_frame.can_dlc`
  - [ ] Copy data bytes to `can_frame.data[]`
  - [ ] Throw `ProtocolException` for invalid frames

- [ ] **1.4 Implement from_socketcan() conversion**
  - [ ] Use `FrameBuilder<VariableFrame>` for construction
  - [ ] Extract extended flag from `can_id & CAN_EFF_FLAG`
  - [ ] Extract remote flag from `can_id & CAN_RTR_FLAG`
  - [ ] Mask ID bits: `can_id & CAN_EFF_MASK` (extended) or `can_id & CAN_SFF_MASK` (standard)
  - [ ] Set format based on RTR flag (DATA_VARIABLE or REMOTE_VARIABLE)
  - [ ] Copy DLC and data via builder
  - [ ] Builder will throw on validation errors

- [ ] **1.5 Add unit tests**
  - [ ] Create `test/test_socketcan_helpers.cpp`
  - [ ] Test standard ID conversion (to_socketcan → from_socketcan round-trip)
  - [ ] Test extended ID conversion (to_socketcan → from_socketcan round-trip)
  - [ ] Test remote frame conversion
  - [ ] Test data frame conversion with various DLCs (0-8 bytes)
  - [ ] Test error cases with `REQUIRE_THROWS_AS<ProtocolException>()`
  - [ ] Verify CAN flags are set correctly

---

### Phase 2: Bridge Configuration Structure ⬜
**Goal**: Define configuration struct and validation

- [ ] **2.1 Define BridgeConfig struct**
  - [ ] Edit `include/pattern/socket_can_bridge.hpp`
  - [ ] Create `struct BridgeConfig` with fields:
    - [ ] `std::string socketcan_interface` (e.g., "can0", "vcan0")
    - [ ] `std::string usb_device_path` (e.g., "/dev/ttyUSB0")
    - [ ] `SerialBaud serial_baud_rate` (for USBAdapter)
    - [ ] `CANBaud can_baud_rate` (for USB adapter ConfigFrame)
    - [ ] `CANMode can_mode` (normal, loopback, silent, silent-loopback)
    - [ ] `bool auto_retransmit` (RTX setting)
    - [ ] `std::uint32_t filter_id` (optional, default 0)
    - [ ] `std::uint32_t filter_mask` (optional, default 0)
    - [ ] `std::uint32_t usb_read_timeout_ms` (default 100ms)
    - [ ] `std::uint32_t socketcan_read_timeout_ms` (default 100ms)

- [ ] **2.2 Add configuration validation**
  - [ ] Add `void validate() const` method to BridgeConfig (throws on invalid config)
  - [ ] Validate interface name is not empty (throw `std::invalid_argument`)
  - [ ] Validate USB device path exists (use `access()` syscall, throw `DeviceException`)
  - [ ] Validate timeout values are reasonable (> 0, < some max)
  - [ ] Validate filter/mask ranges for standard vs extended IDs

- [ ] **2.3 Add default configuration factory**
  - [ ] Add `static BridgeConfig default_config()` method
  - [ ] Set sensible defaults (vcan0, /dev/ttyUSB0, 2Mbps serial, 1Mbps CAN, normal mode)

---

### Phase 3: SocketCAN Socket Management ⬜
**Goal**: Encapsulate Linux SocketCAN socket lifecycle

- [ ] **3.1 Add socket management methods to SocketCANBridge**
  - [ ] Declare private method `int open_socketcan_socket(const std::string& interface)` (throws on error)
  - [ ] Declare private method `void close_socketcan_socket()` (throws on error)
  - [ ] Declare private method `void set_socketcan_timeouts()` (throws on error)
  - [ ] Add member variable `int socketcan_fd_ = -1`

- [ ] **3.2 Implement open_socketcan_socket()**
  - [ ] Create socket: `socket(PF_CAN, SOCK_RAW, CAN_RAW)`
  - [ ] Throw `DeviceException` on socket creation error
  - [ ] Get interface index via `ioctl(fd, SIOCGIFINDEX, &ifr)` with `struct ifreq`
  - [ ] Bind socket to interface using `struct sockaddr_can`
  - [ ] Return socket FD on success, throw on failure

- [ ] **3.3 Implement set_socketcan_timeouts()**
  - [ ] Set `SO_RCVTIMEO` socket option using `setsockopt()`
  - [ ] Use `struct timeval` from `config_.socketcan_read_timeout_ms`
  - [ ] Throw `DeviceException` on setsockopt failure

- [ ] **3.4 Implement close_socketcan_socket()**
  - [ ] Check if `socketcan_fd_` is valid (>= 0)
  - [ ] Call `close(socketcan_fd_)`
  - [ ] Set `socketcan_fd_ = -1`
  - [ ] Throw `DeviceException` on close failure

- [ ] **3.5 Add socket cleanup to destructor**
  - [ ] Ensure `close_socketcan_socket()` is called in destructor
  - [ ] Handle errors gracefully (log but don't throw)

---

### Phase 4: USB Adapter Integration ⬜
**Goal**: Initialize and configure USB adapter for bridge operation

- [ ] **4.1 Add USB adapter member and methods**
  - [ ] Add member `std::shared_ptr<USBAdapter> adapter_`
  - [ ] Declare private method `void configure_usb_adapter()` (throws on error)
  - [ ] Declare private method `void verify_adapter_config()` (throws on error)

- [ ] **4.2 Implement adapter initialization in constructor**
  - [ ] Create USBAdapter: `std::make_shared<USBAdapter>(config_.usb_device_path, config_.serial_baud_rate)`
  - [ ] USBAdapter constructor already throws on open failure
  - [ ] Store in `adapter_` member

- [ ] **4.3 Implement configure_usb_adapter()**
  - [ ] Create ConfigFrame using FrameBuilder:
    - [ ] Set CAN baud rate from config
    - [ ] Set CAN mode from config
    - [ ] Set auto-retransmit flag from config
    - [ ] Set filter ID and mask from config
  - [ ] Send ConfigFrame via `adapter_->send_frame(config_frame)` (throws on error)
  - [ ] Wait for echo/ACK by reading back a ConfigFrame
  - [ ] Validate echoed config matches sent config
  - [ ] Throw `DeviceException` on mismatch

- [ ] **4.4 Test adapter configuration**
  - [ ] Create unit test with mock adapter
  - [ ] Verify ConfigFrame is sent correctly
  - [ ] Verify error handling for send failures

---

### Phase 5: Statistics Tracking Infrastructure ⬜
**Goal**: Add lightweight performance monitoring

- [ ] **5.1 Define BridgeStatistics struct**
  - [ ] Create `struct BridgeStatistics` in `socket_can_bridge.hpp`
  - [ ] Add atomic counters:
    - [ ] `std::atomic<uint64_t> usb_rx_frames{0}` (frames received from USB)
    - [ ] `std::atomic<uint64_t> usb_tx_frames{0}` (frames sent to USB)
    - [ ] `std::atomic<uint64_t> socketcan_rx_frames{0}` (frames received from SocketCAN)
    - [ ] `std::atomic<uint64_t> socketcan_tx_frames{0}` (frames sent to SocketCAN)
    - [ ] `std::atomic<uint64_t> usb_rx_errors{0}` (USB receive errors)
    - [ ] `std::atomic<uint64_t> usb_tx_errors{0}` (USB send errors)
    - [ ] `std::atomic<uint64_t> socketcan_rx_errors{0}` (SocketCAN receive errors)
    - [ ] `std::atomic<uint64_t> socketcan_tx_errors{0}` (SocketCAN send errors)
    - [ ] `std::atomic<uint64_t> conversion_errors{0}` (frame conversion failures)

- [ ] **5.2 Add statistics member and methods**
  - [ ] Add member `BridgeStatistics stats_`
  - [ ] Declare `BridgeStatistics get_statistics() const` (returns snapshot)
  - [ ] Declare `void reset_statistics()` (zeros all counters)

- [ ] **5.3 Implement statistics methods**
  - [ ] Implement `get_statistics()`: copy all atomic values to non-atomic struct
  - [ ] Implement `reset_statistics()`: store 0 to all atomics
  - [ ] Add `to_string()` method for human-readable stats

---

### Phase 6: USB → SocketCAN Forwarding Thread ⬜
**Goal**: Implement thread that reads from USB and forwards to SocketCAN

- [ ] **6.1 Declare thread method and member**
  - [ ] Declare private method `void usb_to_socketcan_loop()`
  - [ ] Add member `std::thread usb_to_socketcan_thread_`

- [ ] **6.2 Implement usb_to_socketcan_loop()**
  - [ ] Add while loop: `while (running_.load(std::memory_order_relaxed))`
  - [ ] Wrap body in try-catch block
  - [ ] Call `adapter_->receive_variable_frame(config_.usb_read_timeout_ms)`
  - [ ] Catch `TimeoutException`: continue loop (expected behavior)
  - [ ] Catch other exceptions: increment `stats_.usb_rx_errors`, log error, continue
  - [ ] On success: increment `stats_.usb_rx_frames`
  - [ ] Convert to `can_frame` via `SocketCANHelper::to_socketcan()` (may throw)
  - [ ] Write to SocketCAN: `write(socketcan_fd_, &cf, sizeof(struct can_frame))`
  - [ ] Handle write error: throw `DeviceException`, caught in outer try-catch
  - [ ] On success: increment `stats_.socketcan_tx_frames`

- [ ] **6.3 Add error logging**
  - [ ] Include `<iostream>` for stderr logging
  - [ ] Log errors with context (USB read, conversion, SocketCAN write)
  - [ ] Use `Result::describe()` for detailed error messages

- [ ] **6.4 Add thread exception handling**
  - [ ] Wrap entire loop in try-catch
  - [ ] Catch and log any unexpected exceptions
  - [ ] Set `running_ = false` on fatal errors

---

### Phase 7: SocketCAN → USB Forwarding Thread ⬜
**Goal**: Implement thread that reads from SocketCAN and forwards to USB

- [ ] **7.1 Declare thread method and member**
  - [ ] Declare private method `void socketcan_to_usb_loop()`
  - [ ] Add member `std::thread socketcan_to_usb_thread_`

- [ ] **7.2 Implement socket read with timeout**
  - [ ] Use `select()` or `poll()` to wait for socket readability
  - [ ] Set timeout from `config_.socketcan_read_timeout_ms`
  - [ ] Handle timeout: continue loop (check `running_` again)
  - [ ] Handle select error: log and continue

- [ ] **7.3 Implement socketcan_to_usb_loop()**
  - [ ] Add while loop: `while (running_.load(std::memory_order_relaxed))`
  - [ ] Wrap body in try-catch block
  - [ ] Call `select()` with timeout to check socket
  - [ ] If readable, call `read(socketcan_fd_, &cf, sizeof(struct can_frame))`
  - [ ] On read error: throw `DeviceException`, caught and logged
  - [ ] On success: increment `stats_.socketcan_rx_frames`
  - [ ] Convert to VariableFrame via `SocketCANHelper::from_socketcan()` (may throw)
  - [ ] Send via `adapter_->send_frame(frame)` (may throw)
  - [ ] Catch exceptions: increment appropriate error counter, log, continue
  - [ ] On success: increment `stats_.usb_tx_frames`

- [ ] **7.4 Add error handling**
  - [ ] Same logging and exception handling as USB → SocketCAN thread

---

### Phase 8: Bridge Lifecycle Management ⬜
**Goal**: Implement start(), stop(), and destructor

- [ ] **8.1 Add running flag and constructor**
  - [ ] Add member `std::atomic<bool> running_{false}`
  - [ ] Implement constructor:
    - [ ] Store `BridgeConfig` copy
    - [ ] Initialize USB adapter (constructor throws on failure)
    - [ ] Open SocketCAN socket (throws on failure)
    - [ ] Set socket timeouts (throws on failure)
    - [ ] Configure USB adapter (send ConfigFrame, throws on failure)
    - [ ] All exceptions propagate to caller

- [ ] **8.2 Implement start() method**
  - [ ] Check if already running: throw `std::logic_error` if `running_ == true`
  - [ ] Set `running_ = true`
  - [ ] Spawn `usb_to_socketcan_thread_` with `usb_to_socketcan_loop()`
  - [ ] Spawn `socketcan_to_usb_thread_` with `socketcan_to_usb_loop()`

- [ ] **8.3 Implement stop() method**
  - [ ] Check if not running: return early if `running_ == false`
  - [ ] Set `running_ = false`
  - [ ] Join `usb_to_socketcan_thread_` (block until complete)
  - [ ] Join `socketcan_to_usb_thread_`
  - [ ] Close SocketCAN socket
  - [ ] Return statistics snapshot (or throw on join failure)

- [ ] **8.4 Implement destructor**
  - [ ] Call `stop()` if `running_ == true`
  - [ ] Catch and log any exceptions (destructors should not throw)
  - [ ] Ensure all resources are cleaned up

- [ ] **8.5 Add query methods**
  - [ ] Implement `bool is_running() const` (return `running_.load()`)
  - [ ] Implement `BridgeConfig get_config() const` (return copy of config)

---

### Phase 9: Unit Testing ⬜
**Goal**: Test bridge components in isolation

- [ ] **9.1 Create test file**
  - [ ] Create `test/test_socketcan_bridge.cpp`
  - [ ] Include Catch2 headers
  - [ ] Include SocketCANBridge header

- [ ] **9.2 Test configuration validation**
  - [ ] Test `BridgeConfig::validate()` with valid config
  - [ ] Test validation failure for empty interface name
  - [ ] Test validation failure for invalid USB device path
  - [ ] Test validation failure for invalid timeout values

- [ ] **9.3 Test statistics tracking**
  - [ ] Create bridge instance
  - [ ] Verify initial statistics are zero
  - [ ] Manually increment counters
  - [ ] Verify `get_statistics()` returns correct values
  - [ ] Test `reset_statistics()` zeros counters

- [ ] **9.4 Test lifecycle**
  - [ ] Test `start()` returns success on first call
  - [ ] Test `start()` returns error when already running
  - [ ] Test `stop()` returns statistics
  - [ ] Test `is_running()` reflects state correctly
  - [ ] Test destructor cleans up gracefully

- [ ] **9.5 Mock-based thread tests (optional)**
  - [ ] Consider mocking USBAdapter and SocketCAN socket
  - [ ] Test loop logic in isolation
  - [ ] Verify error handling paths

---

### Phase 10: Integration Testing with vcan0 ⬜
**Goal**: Test bridge with real SocketCAN virtual interface

- [ ] **10.1 Set up vcan0 interface**
  - [ ] Load vcan module: `sudo modprobe vcan`
  - [ ] Create vcan0: `sudo ip link add dev vcan0 type vcan`
  - [ ] Bring up interface: `sudo ip link set up vcan0`
  - [ ] Document setup in test file or README

- [ ] **10.2 Create integration test**
  - [ ] Create `test/test_socketcan_bridge_integration.cpp`
  - [ ] Check if vcan0 exists before running test (skip if not available)
  - [ ] Use `/dev/ttyUSB0` (or loopback serial port if available)

- [ ] **10.3 Test bidirectional forwarding**
  - [ ] Create bridge with vcan0 and USB device
  - [ ] Start bridge
  - [ ] Send CAN frame on vcan0 using `cansend` or `can-utils`
  - [ ] Verify frame appears on USB side
  - [ ] Send frame on USB side
  - [ ] Verify frame appears on vcan0 using `candump`
  - [ ] Stop bridge and verify statistics

- [ ] **10.4 Test error recovery**
  - [ ] Simulate USB read timeout (disconnect device temporarily)
  - [ ] Verify bridge continues running
  - [ ] Verify error counters increment
  - [ ] Reconnect device and verify recovery

- [ ] **10.5 Test graceful shutdown**
  - [ ] Start bridge
  - [ ] Send SIGINT signal
  - [ ] Verify bridge stops cleanly
  - [ ] Verify all threads join
  - [ ] Verify sockets are closed

---

### Phase 11: Example Script & Documentation ⬜
**Goal**: Provide user-facing example and documentation

- [ ] **11.1 Create example script**
  - [ ] Create `scripts/socketcan_bridge_demo.cpp`
  - [ ] Include command-line argument parsing (use `script_utils.hpp` pattern):
    - [ ] `-i, --interface`: SocketCAN interface (default: vcan0)
    - [ ] `-d, --device`: USB device path (default: /dev/ttyUSB0)
    - [ ] `-s, --serial-baud`: Serial baud rate (default: 2000000)
    - [ ] `-b, --can-baud`: CAN baud rate (default: 1000000)
    - [ ] `-m, --mode`: CAN mode (default: normal)
    - [ ] `-h, --help`: Show usage

- [ ] **11.2 Implement demo application**
  - [ ] Parse command-line arguments
  - [ ] Create BridgeConfig from arguments
  - [ ] Create SocketCANBridge instance
  - [ ] Start bridge
  - [ ] Install SIGINT handler
  - [ ] Run until SIGINT received
  - [ ] Stop bridge and print statistics
  - [ ] Clean up and exit

- [ ] **11.3 Add CMakeLists.txt entry**
  - [ ] Add executable target for `socketcan_bridge_demo`
  - [ ] Link against waveshare library
  - [ ] Add install rule (optional)

- [ ] **11.4 Update README.md**
  - [ ] Add "SocketCAN Bridge" section
  - [ ] Document purpose and architecture
  - [ ] Document configuration options
  - [ ] Provide usage example:
    ```bash
    # Set up vcan0
    sudo modprobe vcan
    sudo ip link add dev vcan0 type vcan
    sudo ip link set up vcan0
    
    # Run bridge
    ./build/scripts/socketcan_bridge_demo -i vcan0 -d /dev/ttyUSB0
    
    # In another terminal, send/receive frames
    cansend vcan0 123#DEADBEEF
    candump vcan0
    ```
  - [ ] Document statistics output format

- [ ] **11.5 Update .github/copilot-instructions.md**
  - [ ] Add SocketCAN Bridge section to architecture overview
  - [ ] Document threading model and thread-safety
  - [ ] Document conversion helpers
  - [ ] Add example usage patterns
  - [ ] Document statistics tracking

---

### Phase 12: Advanced Features (Future) ⬜
**Goal**: Plan and document future enhancements (don't implement yet)

- [ ] **12.1 Document optimization opportunities**
  - [ ] Zero-copy I/O using `recvmsg()` with `MSG_PEEK`
  - [ ] Batched socket writes (accumulate frames, send multiple at once)
  - [ ] Lock-free ring buffers (SPSC queues) instead of threads
  - [ ] Hardware timestamping with `SO_TIMESTAMP`
  - [ ] Memory pooling for VariableFrame objects

- [ ] **12.2 Document CAN filter improvements**
  - [ ] Apply filters at SocketCAN socket level (`CAN_RAW_FILTER` socket option)
  - [ ] Dynamic filter updates without restarting bridge
  - [ ] Multiple filter/mask pairs

- [ ] **12.3 Document ROS2 integration plan**
  - [ ] Lifecycle node wrapper for SocketCANBridge
  - [ ] ROS parameters for BridgeConfig
  - [ ] Diagnostics integration (publish statistics)
  - [ ] Service interfaces (dynamic config updates, statistics queries)
  - [ ] Integration with `ros2_socketcan` package

- [ ] **12.4 Document CANopen middleware plan**
  - [ ] SDO (Service Data Object) protocol handling
  - [ ] PDO (Process Data Object) protocol handling
  - [ ] NMT (Network Management) operations
  - [ ] Object dictionary management
  - [ ] Integration with SocketCAN bridge

---

### Phase 13: Code Review & Cleanup ⬜
**Goal**: Polish code before merge

- [ ] **13.1 Code formatting**
  - [ ] Run uncrustify on all new files
  - [ ] Verify consistent style with existing codebase
  - [ ] Fix any formatting issues

- [ ] **13.2 Documentation review**
  - [ ] Verify all public methods have doxygen comments
  - [ ] Verify README is up-to-date
  - [ ] Verify example code is correct
  - [ ] Fix any typos or unclear explanations

- [ ] **13.3 Error handling review**
  - [ ] Verify all errors throw appropriate exception types
  - [ ] Verify error messages are descriptive
  - [ ] Verify exceptions provide proper context
  - [ ] Add missing error cases

- [ ] **13.4 Performance review**
  - [ ] Profile forwarding loops (check for hot spots)
  - [ ] Verify no unnecessary allocations in hot paths
  - [ ] Verify atomic operations use appropriate memory order
  - [ ] Consider micro-optimizations if needed

- [ ] **13.5 Test coverage review**
  - [ ] Run all tests and verify 100% pass rate
  - [ ] Identify any untested code paths
  - [ ] Add missing tests
  - [ ] Verify tests are deterministic and repeatable

---

### Phase 14: Merge & Deployment ⬜
**Goal**: Merge to main and prepare for release

- [ ] **14.1 Branch management**
  - [ ] Ensure all commits are on `socketcan` branch
  - [ ] Rebase on latest `main` if needed
  - [ ] Resolve any merge conflicts

- [ ] **14.2 Final testing**
  - [ ] Run full test suite on clean build
  - [ ] Test on actual hardware with real CAN devices
  - [ ] Test with different CAN baud rates and modes
  - [ ] Test error recovery scenarios

- [ ] **14.3 Create pull request**
  - [ ] Write comprehensive PR description
  - [ ] Reference related issues/prompts
  - [ ] Request code review
  - [ ] Address review comments

- [ ] **14.4 Merge to main**
  - [ ] Squash/merge commits as appropriate
  - [ ] Update CHANGELOG
  - [ ] Tag release version
  - [ ] Update main branch README

- [ ] **14.5 Deployment artifacts (optional)**
  - [ ] Consider creating udev rules for USB device permissions
  - [ ] Consider systemd service file for bridge daemon
  - [ ] Document deployment steps
  - [ ] Create installation script

---

## Implementation Notes

### Key Design Decisions

1. **Immutable USB Configuration**: USB adapter is configured once at startup via ConfigFrame, not during operation. This eliminates runtime validation overhead and mutex contention.

2. **Independent Thread Design**: Two threads with no shared mutable state except atomic `running_` flag. Each thread uses USBAdapter's existing thread-safety (read/write mutexes) and SocketCAN socket (separate read/write).

3. **Static Conversion Helpers**: `SocketCANHelper` follows the pattern of `ChecksumHelper` and `VarTypeHelper` - pure static utilities with zero overhead.

4. **Atomic Statistics**: Lock-free performance counters using `std::atomic<uint64_t>` with relaxed memory ordering for minimum overhead.

5. **Graceful Error Handling**: Errors in forwarding loops are logged and counted but don't stop the bridge. Only fatal errors (socket closed, adapter disconnected) terminate threads.

### Future Optimization Opportunities (Documented, Not Implemented)

1. **Zero-copy I/O**: Use `recvmsg()` with scatter-gather for SocketCAN reads
2. **Batched Operations**: Accumulate multiple frames before socket writes
3. **Lock-free Queues**: Replace threads with SPSC ring buffers for lower latency
4. **Hardware Timestamping**: Use `SO_TIMESTAMP` socket option for precise timing
5. **Memory Pooling**: Pre-allocate VariableFrame objects to reduce allocations

---