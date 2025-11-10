# CANopen API Reference for ROS2 Integration

> **Author**: effibot (andrea.efficace1@gmail.com)  
> **Date**: 2025-11-10  
> **Purpose**: Complete API reference for implementing ROS2 wrapper around waveshare_cpp CANopen layer

## Table of Contents

1. [Overview](#overview)
2. [Architecture Layers](#architecture-layers)
3. [Core Components](#core-components)
4. [SDO Client API](#sdo-client-api)
5. [PDO Manager API](#pdo-manager-api)
6. [CIA402 State Machine](#cia402-state-machine)
7. [Object Dictionary](#object-dictionary)
8. [Enum-First Design](#enum-first-design)
9. [Usage Patterns](#usage-patterns)
10. [ROS2 Mapping Strategy](#ros2-mapping-strategy)

---

## Overview

The **waveshare_cpp CANopen layer** provides a complete implementation of CANopen (CiA 301) and CIA402 (Drives and Motion Control) protocols for motor control over CAN bus. All communication happens through **SocketCAN** (`vcan0`, `can0`, etc.), with USB-CAN adapter abstracted by the bridge layer.

### Key Design Principles

1. **Enum-First Design** - No magic numbers, all constants are type-safe enums
2. **Dependency Injection** - All components use `ICANSocket` interface for testability
3. **Exception-Based Errors** - Clear error handling with typed exceptions
4. **Thread-Safe** - Lock-free atomics for statistics, mutexes for callbacks
5. **Hardware-Independent** - Mock-based testing, no hardware required for development

### Protocol Stack

```
┌─────────────────────────────────────┐
│     ROS2 Wrapper (TO BE BUILT)      │  ← Services, Topics, Actions
├─────────────────────────────────────┤
│    CIA402 State Machine (FSM)       │  ← Motor enable/disable, fault reset
├─────────────────────────────────────┤
│  SDO Client  │    PDO Manager        │  ← Read/write + Real-time data
├──────────────┴───────────────────────┤
│       Object Dictionary (JSON)       │  ← Register mapping
├─────────────────────────────────────┤
│         SocketCAN (Linux)            │  ← can0, vcan0
├─────────────────────────────────────┤
│  SocketCAN Bridge (USB<->CAN)        │  ← USB-CAN-A adapter
└─────────────────────────────────────┘
```

---

## Architecture Layers

### Layer 1: Bridge (USB ↔ SocketCAN)

**Purpose**: Connects Waveshare USB-CAN-A adapter to Linux SocketCAN interface.

**Class**: `SocketCANBridge` (`include/pattern/socketcan_bridge.hpp`)

**Key Features**:
- Bidirectional frame forwarding (USB ↔ CAN)
- Automatic transport detection (direct USB or SocketCAN)
- Configuration via JSON or environment variables
- Thread-safe operation with atomic statistics

**ROS2 Relevance**: Already implemented in `CanopenLifeCycleNode`. No changes needed.

---

### Layer 2: CANopen Communication

**Purpose**: Implements CiA 301 SDO/PDO protocols for object access and real-time data.

**Components**:
- **SDO Client**: Request/response communication for configuration and slow data
- **PDO Manager**: Real-time cyclic data (sensor feedback, motor commands)
- **Object Dictionary**: Register mapping (index/subindex → name, type, access)

**ROS2 Relevance**: Core layer to wrap - all motor control happens here.

---

### Layer 3: CIA402 Motor Control

**Purpose**: Implements CiA 402 drive profile for motor state management.

**Component**: `CIA402FSM` (Finite State Machine)

**Key Features**:
- Motor enable/disable sequences
- Fault detection and reset
- State monitoring (8 states: NOT_READY, FAULT, OPERATION_ENABLED, etc.)
- Operation mode setting (Position, Velocity, Torque modes)

**ROS2 Relevance**: High-level API for motor lifecycle - map to Action Servers.

---

## Core Components

### 1. ICANSocket Interface

**File**: `include/io/can_socket.hpp`

All CANopen components depend on this interface:

```cpp
class ICANSocket {
public:
    virtual bool open(const std::string& interface_name) = 0;
    virtual bool close() = 0;
    virtual bool is_open() const = 0;
    
    virtual bool send(const can_frame& frame) = 0;
    virtual bool receive(can_frame& frame, int timeout_ms) = 0;
    
    virtual bool set_filters(const std::vector<can_filter>& filters) = 0;
    virtual int get_socket_fd() const = 0;
};
```

**Implementations**:
- `RealCANSocket` - Production (actual hardware)
- `MockCANSocket` - Testing (no hardware required)

**ROS2 Usage**: Use `RealCANSocket` with `can0` or `vcan0` interface.

---

### 2. ObjectDictionary

**File**: `include/canopen/object_dictionary.hpp`

Maps CANopen object names to register indices and types.

```cpp
class ObjectDictionary {
public:
    explicit ObjectDictionary(const std::string& json_path);
    
    // Lookup by name
    ObjectEntry get_entry(const std::string& name) const;
    
    // Serialization helpers
    template<typename T>
    std::vector<uint8_t> to_raw(T value) const;
    
    template<typename T>
    T from_raw(const std::vector<uint8_t>& data) const;
};

struct ObjectEntry {
    uint16_t index;
    uint8_t subindex;
    std::string type;        // "uint16", "int32", etc.
    std::string access;      // "rw", "ro", "wo"
    std::string description;
};
```

**Configuration File**: `config/motor_config.json`

```json
{
  "statusword": {
    "index": "0x6041",
    "subindex": 0,
    "type": "uint16",
    "access": "ro",
    "description": "Device status word"
  },
  "position_actual_value": {
    "index": "0x6064",
    "subindex": 0,
    "type": "int32",
    "access": "ro"
  }
}
```

**ROS2 Usage**: Load once during node initialization, share across components.

---

## SDO Client API

**File**: `include/canopen/sdo_client.hpp`

### Constructor

```cpp
SDOClient(
    std::shared_ptr<ICANSocket> socket,
    const ObjectDictionary& dictionary,
    uint8_t node_id
);
```

**Parameters**:
- `socket`: Shared pointer to SocketCAN interface (e.g., `can0`)
- `dictionary`: Reference to object dictionary (loaded from JSON)
- `node_id`: CANopen node ID (1-127)

### Type-Safe Read

```cpp
template<typename T>
T read(const std::string& object_name);
```

**Example**:
```cpp
uint16_t statusword = sdo_client.read<uint16_t>("statusword");
int32_t position = sdo_client.read<int32_t>("position_actual_value");
uint8_t error_reg = sdo_client.read<uint8_t>("error_register");
```

**Supported Types**: `uint8_t`, `uint16_t`, `uint32_t`, `int8_t`, `int16_t`, `int32_t`

**Exceptions**: Throws `CANException` on timeout or protocol error.

### Type-Safe Write

```cpp
template<typename T>
bool write(const std::string& object_name, T value);
```

**Example**:
```cpp
sdo_client.write<uint16_t>("controlword", 0x06);  // Shutdown command
sdo_client.write<int32_t>("target_position", 10000);
sdo_client.write<uint8_t>("modes_of_operation", 0x01);  // Profile Position Mode
```

**Returns**: `true` on success, `false` on failure (also logs error).

### Protocol Details

- **COB-IDs**: TX = `0x600 + node_id`, RX = `0x580 + node_id`
- **Timeout**: Default 1000ms (configurable)
- **Frame Format**: CiA 301 SDO expedited transfer (up to 4 bytes)
- **Thread-Safety**: Not thread-safe - use one SDO client per thread or add mutex

**ROS2 Mapping**: Wrap in **ROS2 Service** for on-demand register access.

---

## PDO Manager API

**File**: `include/canopen/pdo_manager.hpp`

### Constructor

```cpp
PDOManager(std::shared_ptr<ICANSocket> socket);
```

**Parameters**:
- `socket`: Shared pointer to SocketCAN interface (same as SDO)

### Lifecycle

```cpp
bool start();    // Open socket, start receive thread
void stop();     // Stop thread, close socket
bool is_running() const;
```

**Thread Model**: Single receive thread handles all motors on the bus.

### Sending RPDOs (Commands to Motor)

```cpp
bool send_rpdo1(uint8_t node_id, const std::vector<uint8_t>& data);
bool send_rpdo2(uint8_t node_id, const std::vector<uint8_t>& data);
```

**COB-IDs**:
- RPDO1: `0x200 + node_id`
- RPDO2: `0x300 + node_id`

**Typical RPDO1 Mapping** (CIA402):
```
Bytes 0-1: Controlword (uint16_t)
Bytes 2-5: Target Position (int32_t)
```

**Example**:
```cpp
std::vector<uint8_t> rpdo1_data(6);
// Controlword: 0x000F (Enable Operation)
rpdo1_data[0] = 0x0F;
rpdo1_data[1] = 0x00;
// Target Position: 5000 counts
int32_t target = 5000;
std::memcpy(&rpdo1_data[2], &target, 4);

pdo_manager.send_rpdo1(node_id, rpdo1_data);
```

### Receiving TPDOs (Feedback from Motor)

```cpp
using TPDOCallback = std::function<void(const can_frame&)>;

void register_tpdo1_callback(uint8_t node_id, TPDOCallback callback);
void register_tpdo2_callback(uint8_t node_id, TPDOCallback callback);
void unregister_callbacks(uint8_t node_id);
```

**COB-IDs**:
- TPDO1: `0x180 + node_id`
- TPDO2: `0x280 + node_id`

**Typical TPDO1 Mapping** (CIA402):
```
Bytes 0-1: Statusword (uint16_t)
Bytes 2-5: Position Actual Value (int32_t)
```

**Example**:
```cpp
pdo_manager.register_tpdo1_callback(node_id, [](const can_frame& frame) {
    uint16_t statusword;
    int32_t position;
    std::memcpy(&statusword, &frame.data[0], 2);
    std::memcpy(&position, &frame.data[2], 4);
    
    std::cout << "Statusword: 0x" << std::hex << statusword << "\n";
    std::cout << "Position: " << std::dec << position << " counts\n";
});
```

### SYNC Message

```cpp
void send_sync();
```

**Purpose**: Triggers synchronized PDO transmission from all motors.

**COB-ID**: `0x80` (standard SYNC object)

**Usage**: Call periodically (e.g., 10ms) to maintain cyclic PDO communication.

### Statistics

```cpp
struct PDOStatistics {
    uint64_t tpdo1_received;
    uint64_t tpdo2_received;
    uint64_t rpdo1_sent;
    uint64_t rpdo2_sent;
    uint64_t errors;
    double avg_latency_us;
};

PDOStatistics get_statistics() const;
void reset_statistics();
```

**Thread-Safety**: Statistics use atomic counters - lock-free access.

**ROS2 Mapping**: 
- **Topics** for TPDO data (publish feedback)
- **Topics** for RPDO data (subscribe to commands)
- **Diagnostics** for statistics

---

## CIA402 State Machine

**File**: `include/canopen/cia402_fsm.hpp`

### Constructor

```cpp
CIA402FSM(
    SDOClient& sdo_client,
    const ObjectDictionary& dictionary,
    std::chrono::milliseconds state_timeout = std::chrono::milliseconds(1000)
);
```

**Dependencies**: Requires initialized SDO client.

### Motor Control

```cpp
bool enable_operation();   // Full enable sequence: NOT_READY → OPERATION_ENABLED
bool disable_operation();  // Disable voltage, return to SWITCH_ON_DISABLED
bool quick_stop();         // Emergency stop
bool reset_fault();        // Clear fault state
bool shutdown();           // Transition to READY_TO_SWITCH_ON
bool switch_on();          // Transition to SWITCHED_ON
```

**State Transitions** (`enable_operation()` example):
```
NOT_READY_TO_SWITCH_ON  (automatic transition)
    ↓
SWITCH_ON_DISABLED  (send SHUTDOWN command)
    ↓
READY_TO_SWITCH_ON  (send SWITCH_ON command)
    ↓
SWITCHED_ON  (send ENABLE_OPERATION command)
    ↓
OPERATION_ENABLED  ← Motor ready for motion!
```

**Returns**: `true` on success, `false` on timeout or error.

**Timeout**: Configurable per state transition (default 1000ms).

### State Queries

```cpp
State get_current_state(bool force_update = false);
std::string get_current_state_string(bool force_update = false);
bool is_operational();
bool has_fault();
bool has_warning();
bool target_reached();
```

**State Caching**: States are cached to reduce SDO reads. Use `force_update=true` to bypass cache.

### CIA402 States (Enum)

```cpp
enum class State : uint8_t {
    NOT_READY_TO_SWITCH_ON = 0,  // Initializing
    SWITCH_ON_DISABLED = 1,      // Ready but voltage disabled
    READY_TO_SWITCH_ON = 2,      // Ready to power on
    SWITCHED_ON = 3,             // Voltage on, drive disabled
    OPERATION_ENABLED = 4,       // Normal operation
    QUICK_STOP_ACTIVE = 5,       // Quick stop engaged
    FAULT_REACTION_ACTIVE = 6,   // Handling fault
    FAULT = 7,                   // Fault state
    UNKNOWN = 0xFF               // Cannot determine
};
```

**ROS2 Mapping**: 
- **Action Server** for state transitions (enable, disable, reset_fault)
- **Topic** for current state publishing

---

## Object Dictionary

### Common CIA402 Registers

**File**: `config/motor_config.json`

| Object Name                  | Index  | Type   | Access | Description                               |
| ---------------------------- | ------ | ------ | ------ | ----------------------------------------- |
| `device_type`                | 0x1000 | uint32 | ro     | Device type identifier                    |
| `error_register`             | 0x1001 | uint8  | ro     | Error register byte                       |
| `statusword`                 | 0x6041 | uint16 | ro     | Device status word                        |
| `controlword`                | 0x6040 | uint16 | rw     | Device control word                       |
| `modes_of_operation`         | 0x6060 | int8   | rw     | Operation mode setting                    |
| `modes_of_operation_display` | 0x6061 | int8   | ro     | Current operation mode                    |
| `position_actual_value`      | 0x6064 | int32  | ro     | Actual position (encoder counts)          |
| `velocity_actual_value`      | 0x606C | int32  | ro     | Actual velocity (counts/sec)              |
| `target_position`            | 0x607A | int32  | rw     | Target position for Profile Position Mode |
| `target_velocity`            | 0x60FF | int32  | rw     | Target velocity for Profile Velocity Mode |
| `target_torque`              | 0x6071 | int16  | rw     | Target torque (‰ of rated torque)         |

**Operation Modes** (register `0x6060`):

```cpp
enum class OperationMode : int8_t {
    NO_MODE = 0,
    PROFILE_POSITION = 1,
    PROFILE_VELOCITY = 3,
    PROFILE_TORQUE = 4,
    HOMING = 6,
    INTERPOLATED_POSITION = 7,
    CYCLIC_SYNC_POSITION = 8,
    CYCLIC_SYNC_VELOCITY = 9,
    CYCLIC_SYNC_TORQUE = 10
};
```

---

## Enum-First Design

### Why Enums?

**Problem**: Magic numbers make code unreadable and error-prone.
```cpp
// ❌ BAD: What does 0x6041 mean?
uint16_t status = sdo_client.read<uint16_t>("0x6041");
if (status & 0x0008) { /* What is bit 3? */ }
```

**Solution**: Type-safe enums with helper functions.
```cpp
// ✅ GOOD: Clear intent
uint16_t status = sdo_client.read<uint16_t>("statusword");
if (status & to_mask(StatuswordBit::FAULT)) {
    std::cout << "Motor fault detected!\n";
}
```

### Register Indices

**File**: `include/canopen/cia402_registers.hpp`

```cpp
enum class CIA402Register : uint16_t {
    DEVICE_TYPE = 0x1000,
    ERROR_REGISTER = 0x1001,
    STATUSWORD = 0x6041,
    CONTROLWORD = 0x6040,
    POSITION_ACTUAL_VALUE = 0x6064,
    // ... 86 total registers
};

constexpr uint16_t to_index(CIA402Register reg) {
    return static_cast<uint16_t>(reg);
}
```

### Statusword Bits

**File**: `include/canopen/cia402_constants.hpp`

```cpp
enum class StatuswordBit : uint16_t {
    READY_TO_SWITCH_ON = 0x0001,
    SWITCHED_ON = 0x0002,
    OPERATION_ENABLED = 0x0004,
    FAULT = 0x0008,
    VOLTAGE_ENABLED = 0x0010,
    QUICK_STOP = 0x0020,
    SWITCH_ON_DISABLED = 0x0040,
    WARNING = 0x0080,
    TARGET_REACHED = 0x0400,
    INTERNAL_LIMIT_ACTIVE = 0x0800,
    REMOTE = 0x0200
};

constexpr uint16_t to_mask(StatuswordBit bit) {
    return static_cast<uint16_t>(bit);
}

// Helper functions
bool has_fault(uint16_t statusword);
bool has_warning(uint16_t statusword);
bool target_reached(uint16_t statusword);
State decode_statusword(uint16_t statusword);
std::string state_to_string(State state);
```

### Controlword Commands

```cpp
enum class ControlwordCommand : uint16_t {
    SHUTDOWN = 0x0006,
    SWITCH_ON = 0x0007,
    DISABLE_VOLTAGE = 0x0000,
    QUICK_STOP = 0x0002,
    DISABLE_OPERATION = 0x0007,
    ENABLE_OPERATION = 0x000F,
    FAULT_RESET = 0x0080,
    HALT = 0x0100
};

constexpr uint16_t to_command(ControlwordCommand cmd) {
    return static_cast<uint16_t>(cmd);
}
```

**Usage**:
```cpp
sdo_client.write<uint16_t>("controlword", to_command(ControlwordCommand::ENABLE_OPERATION));
```

### PDO COB-ID Bases

**File**: `include/canopen/pdo_constants.hpp`

```cpp
enum class PDOCobIDBase : uint16_t {
    RPDO1 = 0x200,
    RPDO2 = 0x300,
    RPDO3 = 0x400,
    RPDO4 = 0x500,
    TPDO1 = 0x180,
    TPDO2 = 0x280,
    TPDO3 = 0x380,
    TPDO4 = 0x480,
    SYNC = 0x80
};

constexpr uint16_t to_cob_base(PDOCobIDBase base) {
    return static_cast<uint16_t>(base);
}

// Calculate full COB-ID
inline uint32_t make_cob_id(PDOCobIDBase base, uint8_t node_id) {
    return to_cob_base(base) + node_id;
}
```

---

## Usage Patterns

### Complete Motor Control Example

```cpp
#include "canopen/sdo_client.hpp"
#include "canopen/pdo_manager.hpp"
#include "canopen/cia402_fsm.hpp"
#include "canopen/object_dictionary.hpp"
#include "io/real_can_socket.hpp"

// 1. Initialize components
auto socket = std::make_shared<RealCANSocket>();
socket->open("can0");

ObjectDictionary dict("config/motor_config.json");
SDOClient sdo(socket, dict, 1);  // node_id = 1
CIA402FSM fsm(sdo, dict);
PDOManager pdo(socket);

// 2. Enable motor
if (!fsm.enable_operation()) {
    std::cerr << "Failed to enable motor\n";
    return -1;
}

// 3. Set operation mode (Profile Position)
sdo.write<int8_t>("modes_of_operation", 
    static_cast<int8_t>(OperationMode::PROFILE_POSITION));

// 4. Register PDO feedback callback
pdo.register_tpdo1_callback(1, [](const can_frame& frame) {
    uint16_t statusword;
    int32_t position;
    std::memcpy(&statusword, &frame.data[0], 2);
    std::memcpy(&position, &frame.data[2], 4);
    
    std::cout << "Position: " << position << " counts\n";
});

pdo.start();

// 5. Send position command via PDO
std::vector<uint8_t> rpdo1(6);
rpdo1[0] = 0x0F; rpdo1[1] = 0x00;  // Enable Operation
int32_t target = 10000;
std::memcpy(&rpdo1[2], &target, 4);
pdo.send_rpdo1(1, rpdo1);

// 6. Wait for motion completion
while (!fsm.target_reached()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

std::cout << "Target reached!\n";

// 7. Cleanup
pdo.stop();
fsm.disable_operation();
```

---

## ROS2 Mapping Strategy

### Recommended Package Structure

```
ros2_waveshare/
├── CMakeLists.txt
├── package.xml
├── config/
│   ├── motor_params.yaml          # ROS2 parameters
│   └── motor_config.json          # CANopen object dictionary
├── include/ros2_waveshare/
│   ├── motor_driver_node.hpp      # Main ROS2 node
│   └── canopen_lifecycle.hpp      # Bridge lifecycle node (existing)
├── src/
│   ├── motor_driver_node.cpp
│   └── canopen_lifecycle.cpp
├── launch/
│   ├── motor_driver.launch.py     # Single motor
│   └── multi_motor.launch.py      # Multiple motors
└── msg/                            # Custom messages (if needed)
    └── MotorFeedback.msg
```

### Component Mapping

| waveshare_cpp Component          | ROS2 Interface                            | Purpose                   |
| -------------------------------- | ----------------------------------------- | ------------------------- |
| `SDOClient::read()`              | **Service** `~/sdo/read`                  | On-demand register access |
| `SDOClient::write()`             | **Service** `~/sdo/write`                 | Configuration changes     |
| `PDOManager` TPDO callbacks      | **Topics** `~/joint_states`, `~/feedback` | Sensor feedback (100Hz+)  |
| `PDOManager` RPDO send           | **Topics** `~/command` (subscribe)        | Motor commands            |
| `CIA402FSM::enable_operation()`  | **Action** `~/enable`                     | Motor startup sequence    |
| `CIA402FSM::reset_fault()`       | **Action** `~/reset_fault`                | Fault recovery            |
| `CIA402FSM::get_current_state()` | **Topic** `~/state` (publish)             | State monitoring          |
| `SocketCANBridge`                | **Lifecycle Node** (existing)             | USB-CAN management        |

### Suggested ROS2 Messages

**Standard Messages** (prefer these):
- `sensor_msgs/JointState` - Position, velocity, effort feedback
- `std_msgs/Float64` - Simple command values
- `diagnostic_msgs/DiagnosticArray` - Error reporting

**Custom Messages** (if needed):
```
# MotorFeedback.msg
std_msgs/Header header
uint8 node_id
uint16 statusword
int32 position_counts
int32 velocity_counts_per_sec
int16 current_ma
string state_name
```

### Launch File Pattern

```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # Bridge node (already exists)
        Node(
            package='ros2_waveshare',
            executable='canopen_lifecycle_node',
            parameters=[{'usb_device': '/dev/ttyUSB0',
                        'can_interface': 'can0'}]
        ),
        
        # Motor driver node (TO BE BUILT)
        Node(
            package='ros2_waveshare',
            executable='motor_driver_node',
            parameters=[{
                'node_id': 1,
                'can_interface': 'can0',
                'config_file': 'config/motor_config.json',
                'control_rate': 100.0  # Hz
            }],
            remappings=[
                ('/motor/joint_states', '/traction_motor_left/joint_states')
            ]
        )
    ])
```

---

## Testing Strategy

### Unit Testing (Mock-Based)

All CANopen components support dependency injection - use `MockCANSocket` for testing without hardware:

```cpp
#include "test/mocks/mock_can_socket.hpp"

TEST_CASE("SDO read succeeds") {
    auto mock_socket = std::make_shared<MockCANSocket>();
    ObjectDictionary dict("config/motor_config.json");
    SDOClient sdo(mock_socket, dict, 1);
    
    // Program mock response
    mock_socket->set_next_response([](const can_frame& request) {
        can_frame response;
        response.can_id = 0x581;  // SDO response
        response.data[0] = 0x4B;  // Upload response
        response.data[4] = 0x37;  // Statusword low byte
        response.data[5] = 0x02;  // Statusword high byte
        return response;
    });
    
    uint16_t status = sdo.read<uint16_t>("statusword");
    REQUIRE(status == 0x0237);
}
```

### Integration Testing (Real Hardware)

See `scripts/test_canopen_hardware.cpp` for complete hardware test suite:

```bash
# Test with real motor
./build/scripts/test_canopen_hardware /dev/ttyUSB0 1

# Test with vcan (mock motor)
./build/scripts/test_canopen_hardware vcan0 127
```

---

## Next Steps for ROS2 Integration

1. ✅ **Understand CANopen API** ← You are here
2. **Create ROS2 message/service/action definitions**
   - Define custom messages for motor feedback
   - Define services for SDO access
   - Define actions for state machine transitions
3. **Implement MotorDriverNode class**
   - Initialize SDO, PDO, FSM components
   - Create ROS2 publishers/subscribers
   - Wire up CANopen callbacks to ROS2 topics
4. **Add parameter handling**
   - Load motor configuration from ROS2 parameters
   - Support dynamic reconfiguration
5. **Testing & validation**
   - Unit tests with mock CAN socket
   - Integration tests with real motor
   - Multi-motor coordination tests

---

## References

- **CiA 301**: CANopen Application Layer (SDO, PDO, NMT, SYNC)
- **CiA 402**: CANopen Device Profile for Drives and Motion Control
- **Library Docs**: `README.md`, `doc/PDO_Testing_Guide.md`
- **Test Examples**: `scripts/test_canopen_hardware.cpp`, `test/canopen/*.cpp`
- **Enum Definitions**: `include/canopen/cia402_constants.hpp`, `include/canopen/cia402_registers.hpp`

---

**Last Updated**: 2025-11-10  
**Status**: CANopen layer complete, ready for ROS2 wrapper implementation
