# Waveshare USB-CAN-A C++ Library - AI Coding Instructions

## Project Overview

A type-safe C++ library for Waveshare USB-CAN-A adapters, implementing **State-First Architecture** with CRTP patterns. Provides three major functional layers:

1. **Frame Layer**: Serial protocol for USB-CAN-A devices (`/dev/ttyUSB*`, Linux `ch341-uart` driver)
2. **Bridge Layer**: Bidirectional USB ↔ SocketCAN translation (fully implemented)
3. **CANopen Layer**: CIA402 motor control with SDO/PDO/NMT support (✅ COMPLETE - 2025-11-10)

**Current State:** All layers complete. 132 passing tests (100%, 0.19s runtime). Hardware-independent testing via dependency injection. Thread-safety verified - NO DEADLOCKS POSSIBLE. **CANopen implementation complete with enum-first design** - all magic numbers replaced with type-safe enums.

**Error Handling:** Exception-based with typed hierarchy (WaveshareException → ProtocolException/DeviceException/TimeoutException/CANException).

**Thread Safety:** Three-mutex pattern in USBAdapter (state/write/read), lock-free atomics in SocketCANBridge/PDOManager.

## Architecture Overview

### State-First Design Philosophy (Critical!)

**Core Principle**: Frames are stateful objects that generate protocol buffers on-demand—never store buffers directly:
- **State objects** (`CoreState`, `DataState`, `ConfigState`) hold runtime values (CAN ID, DLC, data, baud rates, etc.)
- **No persistent buffers** - serialization happens on-demand via `serialize()`
- **Getters/setters modify state only** - no direct buffer manipulation
- Protocol bytes are generated during `impl_serialize()` and parsed during `impl_deserialize()`

```cpp
// State-first pattern example
FixedFrame frame;
frame.set_id(0x123);              // Modifies data_state_.can_id
frame.set_data({0x11, 0x22});     // Modifies data_state_.data vector
auto buffer = frame.serialize();   // Generates 20-byte protocol buffer on-demand
```

### CRTP Interface Hierarchy

```
CoreInterface<Frame>           ← CoreState: can_version, type
├── DataInterface<Frame>       ← DataState: format, can_id, dlc, data
│   ├── FixedFrame            ← 20-byte fixed frames
│   └── VariableFrame         ← 5-15 byte variable frames
└── ConfigInterface<Frame>     ← ConfigState: baud, mode, filter, mask
    └── ConfigFrame           ← 20-byte config frames
```

**Key Pattern**: Interfaces use `derived()` CRTP helper to call frame-specific `impl_*()` methods:
```cpp
template<typename Frame>
std::vector<std::uint8_t> CoreInterface<Frame>::serialize() const {
    return derived().impl_serialize();  // Calls FixedFrame::impl_serialize(), etc.
}
```

### USBAdapter: Thread-Safe Serial Communication

`USBAdapter` (`include/pattern/usb_adapter.hpp`) provides frame-level API over serial I/O:
- **Thread-safety**: `state_mutex_` (shared_mutex) protects open/configured state, `write_mutex_`/`read_mutex_` serialize I/O
- **Lifecycle**: Constructor auto-opens/configures port, destructor auto-closes
- **POSIX termios2**: Uses custom baud rates (up to 2Mbps) via `TCGETS2`/`TCSETS2` ioctls
- **Frame API**: `send_frame<T>()`, `receive_fixed_frame()`, `receive_variable_frame()` - all thread-safe
- **Signal handling**: Static `stop_flag` (atomic) set by `SIGINT`, checked via `should_stop()`

```cpp
// Typical usage (see scripts/wave_reader.cpp)
USBAdapter adapter("/dev/ttyUSB0", SerialBaud::BAUD_2M);  // Auto-opens & configures
auto frame = adapter.receive_variable_frame(1000);        // 1s timeout (throws on error)
std::cout << frame.to_string() << "\n";
```

### Frame Types & Protocol

Three frame types implement Waveshare serial protocol (see `README.md` and `doc/diagrams/*.md`):

1. **FixedFrame** (20 bytes): `[START][HEADER][TYPE][CAN_VERS][FORMAT][ID(4)][DLC][DATA(8)][RESERVED][CHECKSUM]`
   - Always 8-byte DATA field (pad with zeros if DLC < 8)
   - Checksum computed during serialization via `ChecksumHelper::compute()`

2. **VariableFrame** (5-15 bytes): `[START][TYPE][ID(2/4)][DATA(0-8)][END]`
   - Dynamic size: ID is 2 bytes (standard) or 4 bytes (extended), DATA is 0-8 bytes
   - TYPE byte encodes: `[0xC0][IsExtended][Format][DLC(4 bits)]` - computed via `VarTypeHelper::encode()`

3. **ConfigFrame** (20 bytes): USB adapter settings (baud rate, mode, filters, masks)
   - Sent to adapter via `send_frame(config_frame)` to configure CAN bus parameters
   - See `include/frame/config_frame.hpp` for baud rate, mode, filter/mask settings

### Critical Implementation Patterns

#### 1. Impl Methods (CRTP Implementation Points)
Frame-specific logic lives in `impl_*()` methods in `src/*.cpp`:
```cpp
// In FixedFrame class (include/frame/fixed_frame.hpp)
std::vector<std::uint8_t> impl_serialize() const;      // Generate 20-byte buffer from state
void impl_deserialize(span<const std::uint8_t>);        // Parse buffer into state (throws on error)
std::size_t impl_serialized_size() const;               // Return 20 (constant)
bool impl_is_extended() const;                          // Check if CAN_VERS is EXT_*
```

#### 2. FrameTraits System (Compile-Time Metadata)
`include/template/frame_traits.hpp` provides layout offsets and type predicates:
```cpp
// Layout usage (all offsets are static constexpr)
using Layout = FrameTraits<FixedFrame>::Layout;
buffer[Layout::START] = 0xAA;        // byte 0
buffer[Layout::ID] = id_byte;        // byte 5
buffer[Layout::CHECKSUM] = checksum; // byte 19

// Type predicates for SFINAE
static_assert(is_data_frame_v<FixedFrame>);
static_assert(has_checksum_v<ConfigFrame>);
```

#### 3. Exception-Based Error Handling
All error conditions throw typed exceptions:
```cpp
// Exception hierarchy
class WaveshareException : public std::runtime_error {
    Status status_;  // Original error code for categorization
};
class ProtocolException : public WaveshareException {};  // Frame/protocol errors
class DeviceException : public WaveshareException {};    // I/O errors
class TimeoutException : public WaveshareException {};   // Timeout errors

// Usage in deserialization
void impl_deserialize(span<const std::uint8_t> buffer) {
    if (buffer.size() != 20) {
        throw ProtocolException(Status::WBAD_LENGTH, "deserialize");
    }
    auto checksum = ChecksumHelper::compute(buffer, ...);
    if (!ChecksumHelper::validate(buffer, pos, ...)) {
        throw ProtocolException(Status::WBAD_CHECKSUM, "deserialize");
    }
    // ... parse fields
}

// Catching exceptions
try {
    auto frame = adapter.receive_variable_frame(timeout);
    // process frame
} catch (const TimeoutException&) {
    // Expected, retry
} catch (const ProtocolException& e) {
    std::cerr << "Protocol error: " << e.what() << "\n";
    // Status available via e.status()
}
```

#### 4. Serialization Helpers (Pure Static Utilities)
`include/interface/serialization_helpers.hpp` provides stateless helpers:
- `ChecksumHelper::compute(buffer, start, end)` - Sum bytes in range, return LSB
- `ChecksumHelper::validate(buffer, pos, start, end)` - Verify stored checksum
- `VarTypeHelper::encode(is_ext, format, dlc)` - Build VariableFrame TYPE byte
- `VarTypeHelper::decode(type_byte)` - Extract is_extended, format, dlc from TYPE

**Design Note**: These replaced the previous dirty-bit tracking pattern for simpler, stateless validation.

### Builder Pattern (Validated Construction)

`FrameBuilder<Frame>` (`include/pattern/frame_builder.hpp`) uses `FrameBuilderState` to accumulate parameters:
```cpp
auto frame = FrameBuilder<FixedFrame>()
    .with_can_version(CANVersion::STD_FIXED)
    .with_format(Format::DATA_FIXED)
    .with_id(0x123)
    .with_data({0x11, 0x22})
    .finalize()    // Sets state_.finalized = true
    .build();      // Validates required fields, constructs FixedFrame

// SFINAE restricts methods:
.with_baud_rate(...)  // Only available for ConfigFrame
.with_id(...)         // Only available for DataInterface frames
```

**Validation Strategy**: Type-safe enums and setter methods prevent invalid states during frame construction.

## Future Work / Next Phase

### SocketCAN Bridge ✅ COMPLETED
**Status**: Fully implemented and tested (see `doc/REFACTORING_COMPLETE.md`)

### CANopen Middleware ✅ COMPLETED (2025-11-10)
**Status**: All CANopen features implemented with enum-first design
- ✅ SDO (Service Data Object) client with type-safe read/write
- ✅ PDO (Process Data Object) manager with multi-motor support
- ✅ CIA402 Finite State Machine for motor control
- ✅ Object Dictionary with JSON configuration
- ✅ Comprehensive enum types (no magic numbers)
- ✅ Hardware testing script (`test_canopen_hardware.cpp`)
- See `doc/CANOPEN_API_REFERENCE.md` for complete API documentation

### ROS2 Integration (NEXT PHASE)
**Status**: Planned - ready for implementation
- ROS2 wrapper package for CANopen motor control
- Lifecycle node integration
- Standard ROS2 message interfaces (JointState, PID, etc.)
- Multi-motor coordination
- See workspace for existing partial implementation in `ros2_waveshare` package

## Important Conventions

### ID Endianness (Critical for Wire Format)
- CAN IDs stored **little-endian** in protocol: ID `0x00000123` → `[0x23][0x01][0x00][0x00]`
- Standard ID: 11 bits (max `0x7FF`), Extended ID: 29 bits (max `0x1FFFFFFF`)
- VariableFrame ID size: 2 bytes (standard) or 4 bytes (extended)

### DLC vs Data Size
- **DLC**: Number of **valid** data bytes (0-8), stored in protocol
- **FixedFrame**: DATA field always 8 bytes in buffer (pad with `0x00` if DLC < 8)
- **VariableFrame**: DATA field size equals DLC (no padding)

### Terminology Mapping
- **Protocol doc** (`FRAME_TYPE`) → **Library** (`CANVersion` enum: `STD_FIXED`, `EXT_VARIABLE`, etc.)
- **Protocol doc** (`Frame Type`) → **Library** (`Type` enum: `DATA_FIXED`, `CONF_VARIABLE`, etc.)

### Dependencies & Standards
- **Namespace**: All code in `namespace USBCANBridge`
- **C++ Standard**: C++17 (uses `std::optional`, `if constexpr`, structured bindings)
- **Boost**: Uses `boost::span` for view semantics (aliased `using namespace boost`)

## Development Workflow

### Build & Test
```bash
# Configure (one-time or after CMakeLists.txt changes)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build (or use VS Code task: "CMake: Build")
cmake --build build --config Debug -j 8

# Run tests (Catch2 auto-discovered via CTest)
cd build && ctest --output-on-failure

# Run specific test
./build/test/test_fixed_frame

# Run example scripts (requires USB device)
./build/scripts/wave_reader -d /dev/ttyUSB0 -s 2000000 -b 1000000 -f variable
./build/scripts/wave_writer -d /dev/ttyUSB1 -s 2000000 -b 1000000 -f variable
```

### VS Code Tasks
- **CMake: Configure** - Run CMake configuration
- **CMake: Build** - Build all targets (default build task)
- **CMake: Clean** - Clean build artifacts
- **Build and Run Reader** - Build and launch wave_reader with default args
- **Build and Run Writer** - Build and launch wave_writer with default args

### Testing with Catch2
- Tests in `test/*.cpp`, using Catch2 v3 (auto-fetched via `FetchContent`)
- Structure: `TEST_CASE("Description", "[tag]")` with fixtures for shared setup
- Assertions: `REQUIRE(expr)` (fails test), `CHECK(expr)` (continues), `REQUIRE_THROWS_AS(expr, Exception)`
- See `test/CATCH2_REFERENCE.md` for patterns
- **Current test count**: 132 passing tests (100%, 0.19s runtime)
- **Test architecture**: Dependency injection with mocks (MockSerialPort, MockCANSocket) - no hardware required
- See `doc/TEST_SUITE_QUICK_REFERENCE.md` for daily use guide

### Testing Hardware
Scripts in `scripts/` provide manual testing tools:
- `wave_reader.cpp` - Read frames from USB adapter (fixed or variable mode)
- `wave_writer.cpp` - Send frames to USB adapter (fixed or variable mode)  
- Both use `script_utils.hpp` for argument parsing and adapter initialization
- Supports SIGINT (Ctrl+C) for graceful shutdown via `USBAdapter::should_stop()`

### Code Coverage
- CMake option `CODE_COVERAGE=ON` exists but implementation pending
- Intended workflow: `cmake -DCODE_COVERAGE=ON` + coverage report generation

### Code Formatting
- Uncrustify config: `uncrustify.cfg` (4-space indent, K&R-like)
- Section comments: `// === Section Name ===` (interfaces), `// * Comment` (impl details)

## Key Files Reference

**Core Architecture**:
- `include/interface/core.hpp` - `CoreInterface<Frame>` with `CoreState` and serialization
- `include/interface/data.hpp` - `DataInterface<Frame>` with `DataState` (ID, DLC, data)
- `include/interface/config.hpp` - `ConfigInterface<Frame>` with `ConfigState`

**Frame Implementations**:
- `include/frame/fixed_frame.hpp` + `src/fixed_frame.cpp` - 20-byte fixed frames
- `include/frame/variable_frame.hpp` + `src/variable_frame.cpp` - 5-15 byte variable frames
- `include/frame/config_frame.hpp` + `src/config_frame.cpp` - USB config frames

**Templates & Utilities**:
- `include/template/frame_traits.hpp` - Compile-time layout offsets and type predicates
- `include/exception/waveshare_exception.hpp` - Exception hierarchy (ProtocolException, DeviceException, etc.)
- `include/interface/serialization_helpers.hpp` - `ChecksumHelper`, `VarTypeHelper` (static)
- `include/pattern/frame_builder.hpp` - Fluent builder with SFINAE-restricted methods

**USB Communication**:
- `include/pattern/usb_adapter.hpp` + `src/usb_adapter.cpp` - Thread-safe serial I/O with frame API
- `scripts/script_utils.hpp` - Shared utilities for wave_reader/wave_writer scripts

**Protocol & Errors**:
- `include/enums/protocol.hpp` - All protocol constants (`START=0xAA`, `CANVersion`, `Format`, etc.)
- `include/enums/error.hpp` - `Status` enum (errors like `WBAD_CHECKSUM`, `WBAD_ID`)

**CANopen Layer** (✅ Complete - 2025-11-10):
- `include/canopen/sdo_client.hpp` - SDO read/write with type-safe interface
- `include/canopen/pdo_manager.hpp` - Multi-motor PDO management with callbacks
- `include/canopen/cia402_fsm.hpp` - State machine for motor enable/disable/fault
- `include/canopen/cia402_constants.hpp` - Type-safe enums (states, bits, commands)
- `include/canopen/cia402_registers.hpp` - Register index enums (86 registers)
- `include/canopen/pdo_constants.hpp` - PDO COB-ID enums
- `include/canopen/object_dictionary.hpp` - JSON-based register mapping
- `config/motor_config.json` - Object dictionary configuration
- `scripts/test_canopen_hardware.cpp` - Complete hardware test suite
- **See `doc/CANOPEN_API_REFERENCE.md` for complete ROS2 integration guide** ⭐

**Documentation**:
- `README.md` - Protocol details, field descriptions, CAN ID construction
- `doc/CANOPEN_API_REFERENCE.md` - **Complete CANopen API for ROS2 wrapper development** ⭐
- `doc/PDO_Testing_Guide.md` - PDO communication patterns and testing
- `doc/SYNCHRONIZATION.md` - Thread-safety patterns
- `doc/diagrams/*.md` - Mermaid diagrams (packet structure, class hierarchy)
- `prompts/INDEX.md` - Development handoff guide

## When Working With This Codebase

1. **Adding frame operations**: Declare `impl_*()` in frame header, define in `src/*.cpp`, expose via interface
2. **Modifying frame structure**: Update `FrameTraits<Frame>` specialization and `Layout` struct in `frame_traits.hpp`
3. **Serialization changes**: Modify `impl_serialize()` in frame's `.cpp` file - remember little-endian for IDs
4. **New validation**: Add to `impl_deserialize()` methods, throw appropriate exception type (`ProtocolException`, `DeviceException`, etc.)
5. **Error handling**: Throw exceptions with descriptive messages and context. Catch at appropriate boundaries (e.g., thread loops, API entry points)
6. **Testing new features**: Add `TEST_CASE` in `test/test_<frame_type>.cpp`, use `REQUIRE_THROWS_AS<ExceptionType>()` for error cases
7. **Thread-safe operations**: Follow USBAdapter pattern - use `std::shared_mutex` for state, separate mutexes for I/O
