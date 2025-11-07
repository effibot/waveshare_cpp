# CANopen Protocol Stack - Class Diagram

Complete CANopen implementation with SDO, PDO, and CIA402 motor control support.

```mermaid

---
config:
  theme: forest
---

classDiagram
    %% ===================================================================
    %% Object Dictionary (CIA301 Core)
    %% ===================================================================
    
    class ObjectDictionary {
        -map~string,OD::Entry~ entries_
        -uint8_t node_id_
        +ObjectDictionary(json_path)
        +get_node_id() uint8_t
        +get_entry(name) OD::Entry
        +get_index(name) uint16_t
        +get_subindex(name) uint8_t
        +get_data_type(name) string
        +to_raw~T~(value) vector~uint8_t~
        +from_raw~T~(data) T
    }
    
    namespace OD {
        class Entry {
            <<struct>>
            +uint16_t index
            +uint8_t subindex
            +string name
            +string data_type
            +string access_type
        }
    }
    
    %% ===================================================================
    %% SDO Client (Service Data Objects - CIA301)
    %% ===================================================================
    
    class SDOClient {
        -shared_ptr~ICANSocket~ socket_
        -ObjectDictionary& dictionary_
        -uint8_t node_id_
        +SDOClient(socket, dict, node_id)
        +write_object(name, data, timeout) bool
        +read_object(name, timeout) vector~uint8_t~
        +write~T~(name, value) bool
        +read~T~(name) T
        +is_open() bool
        -send_frame(frame) bool
        -receive_frame(frame, timeout) bool
        -sdo_tx_cob_id() uint32_t
        -sdo_rx_cob_id() uint32_t
        -create_sdo_write_expedited(...) can_frame
        -create_sdo_read_request(...) can_frame
        -validate_sdo_response(...) bool
    }
    
    %% ===================================================================
    %% PDO Manager (Process Data Objects - CIA301)
    %% ===================================================================
    
    class PDOManager {
        -shared_ptr~ICANSocket~ socket_
        -thread receive_thread_
        -atomic~bool~ running_
        -map~uint32_t,TPDOCallback~ tpdo_callbacks_
        -mutex callback_mutex_
        -map~uint8_t,PDOStatistics~ stats_
        +PDOManager(socket)
        +start() bool
        +stop() void
        +is_running() bool
        +send_rpdo1(node_id, data) bool
        +send_rpdo2(node_id, data) bool
        +register_tpdo1_callback(node_id, callback) void
        +register_tpdo2_callback(node_id, callback) void
        +unregister_callbacks(node_id) void
        +get_statistics(node_id) PDOStatistics
        -receive_loop() void
        -dispatch_tpdo(frame) void
        -send_frame(frame) bool
    }
    
    class PDOStatistics {
        <<struct>>
        +atomic~uint64_t~ rpdo1_sent
        +atomic~uint64_t~ rpdo2_sent
        +atomic~uint64_t~ tpdo1_received
        +atomic~uint64_t~ tpdo2_received
        +chrono::time_point last_tpdo1_time
        +chrono::time_point last_tpdo2_time
    }
    
    class TPDOCallback {
        <<alias>>
        function~void(can_frame)~
    }
    
    %% ===================================================================
    %% PDO Constants (COB-ID Mapping)
    %% ===================================================================
    
    namespace pdo {
        class cob_id {
            <<constants>>
            +RPDO1_BASE = 0x200
            +RPDO2_BASE = 0x300
            +TPDO1_BASE = 0x180
            +TPDO2_BASE = 0x280
        }
        
        class limits {
            <<constants>>
            +MIN_NODE_ID = 1
            +MAX_NODE_ID = 127
            +MAX_PDO_DATA_LENGTH = 8
        }
        
        class PDOType {
            <<enumeration>>
            RPDO1
            RPDO2
            TPDO1
            TPDO2
        }
    }
    
    %% ===================================================================
    %% CIA402 Finite State Machine
    %% ===================================================================
    
    namespace cia402 {
        class State {
            <<enumeration>>
            NOT_READY_TO_SWITCH_ON
            SWITCH_ON_DISABLED
            READY_TO_SWITCH_ON
            SWITCHED_ON
            OPERATION_ENABLED
            QUICK_STOP_ACTIVE
            FAULT_REACTION_ACTIVE
            FAULT
            UNKNOWN
        }
        
        class Command {
            <<enumeration>>
            SHUTDOWN
            SWITCH_ON
            DISABLE_VOLTAGE
            QUICK_STOP
            DISABLE_OPERATION
            ENABLE_OPERATION
            FAULT_RESET
        }
        
        class OperationMode {
            <<enumeration>>
            PROFILE_POSITION = 1
            PROFILE_VELOCITY = 3
            PROFILE_TORQUE = 4
            HOMING = 6
            CYCLIC_SYNC_POSITION = 8
            CYCLIC_SYNC_VELOCITY = 9
            CYCLIC_SYNC_TORQUE = 10
        }
    }
    
    class CIA402FSM {
        +decode_statusword(statusword) State$
        +encode_controlword(cmd) uint16_t$
        +get_transition_command(current, target) Command$
        +state_to_string(state) string$
        +command_to_string(cmd) string$
        +mode_to_string(mode) string$
    }
    
    %% ===================================================================
    %% High-Level CANopen Driver
    %% ===================================================================
    
    class CANopenDriver {
        -shared_ptr~ICANSocket~ socket_
        -ObjectDictionary dictionary_
        -unique_ptr~SDOClient~ sdo_client_
        -unique_ptr~PDOManager~ pdo_manager_
        -uint8_t node_id_
        +CANopenDriver(socket, dict_path, node_id)
        +initialize() bool
        +configure_pdos() bool
        +start_pdos() bool
        +stop() void
        +read_sdo~T~(name) T
        +write_sdo~T~(name, value) bool
        +send_rpdo(data) bool
        +register_tpdo_callback(callback) void
        +get_pdo_statistics() PDOStatistics
    }
    
    %% ===================================================================
    %% Dependencies
    %% ===================================================================
    
    class ICANSocket {
        <<interface>>
        +send(can_frame) ssize_t
        +receive(can_frame) ssize_t
        +get_fd() int
    }
    
    %% ===================================================================
    %% Relationships
    %% ===================================================================
    
    ObjectDictionary *-- OD::Entry : contains
    
    SDOClient o-- ICANSocket : uses
    SDOClient --> ObjectDictionary : uses
    
    PDOManager o-- ICANSocket : uses
    PDOManager *-- PDOStatistics : tracks
    PDOManager ..> TPDOCallback : invokes
    PDOManager ..> pdo::cob_id : uses
    PDOManager ..> pdo::PDOType : uses
    
    CIA402FSM ..> cia402::State : decodes
    CIA402FSM ..> cia402::Command : encodes
    CIA402FSM ..> cia402::OperationMode : uses
    
    CANopenDriver o-- ICANSocket : owns
    CANopenDriver *-- ObjectDictionary : owns
    CANopenDriver *-- SDOClient : owns
    CANopenDriver *-- PDOManager : owns
```

## Protocol Layers

### CIA301 - CANopen Application Layer
- **Object Dictionary**: Standardized parameter access
- **SDO (Service Data Object)**: Read/write parameters (configuration, slow)
- **PDO (Process Data Object)**: Real-time cyclic data (feedback, fast)

### CIA402 - Drive and Motion Control Profile
- **State Machine**: Standardized motor states and transitions
- **Operation Modes**: Position, velocity, torque control modes
- **Standard Objects**: Statusword, controlword, target/actual values

## COB-ID Mapping (CAN Identifier Assignment)

### Standard CANopen Addressing
```
RPDO1 (Commands to Motor):  0x200 + node_id
RPDO2 (Extra Commands):     0x300 + node_id
TPDO1 (Feedback from Motor):0x180 + node_id
TPDO2 (Extra Feedback):     0x280 + node_id
SDO TX (Client → Server):   0x600 + node_id
SDO RX (Server → Client):   0x580 + node_id
```

### Example for Node ID = 1
- RPDO1: 0x201 (host sends position commands)
- TPDO1: 0x181 (motor sends position feedback)
- SDO TX: 0x601 (host sends config requests)
- SDO RX: 0x581 (motor sends config responses)

## Communication Patterns

### SDO Communication (Slow, Confirmed)
```
Client                     Motor
  |  SDO Write (0x601)      |
  |------------------------->|
  |                          |
  |  SDO Response (0x581)    |
  |<-------------------------|
```
**Use case**: Configuration, parameter access, diagnostics

### PDO Communication (Fast, Unconfirmed)
```
Host                       Motor
  |  RPDO1 (0x201)          |
  |------------------------->|  (Position command)
  |                          |
  |  TPDO1 (0x181)           |
  |<-------------------------|  (Position feedback)
```
**Use case**: Real-time control loops, <1ms cycle time

## CIA402 State Machine

```
┌─────────────────────────────────────────────────────┐
│                                                     │
│  POWER ON                                           │
│     ↓                                               │
│  NOT_READY_TO_SWITCH_ON                             │
│     ↓ (automatic)                                   │
│  SWITCH_ON_DISABLED ←────┐                          │
│     ↓ SHUTDOWN           │                          │
│  READY_TO_SWITCH_ON      │ DISABLE_VOLTAGE         │
│     ↓ SWITCH_ON          │                          │
│  SWITCHED_ON ────────────┘                          │
│     ↓ ENABLE_OPERATION                              │
│  OPERATION_ENABLED (motor can move)                 │
│     │                                               │
│     │ FAULT → FAULT_REACTION_ACTIVE → FAULT        │
│     │           ↓ FAULT_RESET                      │
│     └───────────┴──────────────────────────────────┘
│                                                     │
└─────────────────────────────────────────────────────┘
```

## Usage Examples

### Basic SDO Communication
```cpp
// Create socket and dictionary
auto socket = std::make_shared<RealCANSocket>("vcan0", 1000);
ObjectDictionary dict("motor_config.json");

// Create SDO client
SDOClient sdo(socket, dict, 1);

// Read statusword
uint16_t statusword = sdo.read<uint16_t>("statusword");
auto state = cia402::decode_statusword(statusword);

// Write mode of operation
sdo.write<int8_t>("modes_of_operation", 
                  static_cast<int8_t>(cia402::OperationMode::PROFILE_VELOCITY));
```

### PDO Real-Time Control
```cpp
// Create PDO manager
PDOManager pdo(socket);

// Register callback for position feedback
pdo.register_tpdo1_callback(1, [](const can_frame& frame) {
    int32_t position = /* decode frame.data */;
    std::cout << "Position: " << position << std::endl;
});

// Start PDO communication
pdo.start();

// Send position command (RPDO1)
std::vector<uint8_t> cmd = {/* controlword, target_position */};
pdo.send_rpdo1(1, cmd);

// Statistics
auto stats = pdo.get_statistics(1);
std::cout << "TPDO1 received: " << stats.tpdo1_received << std::endl;
```

### Complete Motor Initialization
```cpp
// Create driver
CANopenDriver driver(socket, "motor_config.json", 1);

// Initialize and configure
driver.initialize();
driver.configure_pdos();
driver.start_pdos();

// State machine transitions
uint16_t statusword = driver.read_sdo<uint16_t>("statusword");
auto current_state = cia402::decode_statusword(statusword);

// Transition to OPERATION_ENABLED
if (current_state != cia402::State::OPERATION_ENABLED) {
    auto cmd = cia402::get_transition_command(current_state, 
                                              cia402::State::OPERATION_ENABLED);
    uint16_t controlword = cia402::encode_controlword(cmd);
    driver.write_sdo<uint16_t>("controlword", controlword);
}
```

## Design Benefits

### Dependency Injection
- **ICANSocket**: Works with RealCANSocket (hardware) or MockCANSocket (testing)
- **No Hardware Required**: Integration tests use mock motor responder
- **CI/CD Ready**: All tests run on vcan0 without physical devices

### Thread Safety
- **SDOClient**: Stateless, thread-safe
- **PDOManager**: Lock-free statistics, single receive thread
- **Atomic Counters**: `std::memory_order_relaxed` for performance

### Zero ROS2 Dependencies
- **Pure C++17**: Can be used standalone or with any framework
- **Portable**: Works with any CAN interface (SocketCAN, PEAK, etc.)
- **Reusable**: CANopen stack independent of Waveshare hardware
