# I/O Abstraction Layer - Class Diagram

Hardware abstraction interfaces enabling dependency injection and testing without physical devices.

```mermaid

---
config:
  theme: forest
---

classDiagram
    %% ===================================================================
    %% Serial Port Abstraction
    %% ===================================================================
    
    class ISerialPort {
        <<interface>>
        +write(data, len) ssize_t*
        +read(buffer, len, timeout) ssize_t*
        +close() void*
        +is_open() bool*
        +get_device_path() string*
    }
    
    class RealSerialPort {
        -int fd_
        -string device_path_
        -SerialBaud baud_rate_
        -termios2 tty_
        -bool is_open_
        +RealSerialPort(device, baud)
        +write(data, len) ssize_t
        +read(buffer, len, timeout) ssize_t
        +close() void
        +is_open() bool
        +get_device_path() string
        -open_port() void
        -configure_port() void
    }
    
    class MockSerialPort {
        -queue~vector~uint8_t~~ tx_queue_
        -queue~vector~uint8_t~~ rx_queue_
        -bool open_
        +write(data, len) ssize_t
        +read(buffer, len, timeout) ssize_t
        +push_rx_data(data) void
        +pop_tx_data() vector
        +is_open() bool
    }
    
    %% ===================================================================
    %% CAN Socket Abstraction
    %% ===================================================================
    
    class ICANSocket {
        <<interface>>
        +send(frame) ssize_t*
        +receive(frame) ssize_t*
        +get_fd() int*
        +is_open() bool*
        +close() void*
        +get_interface_name() string*
    }
    
    class RealCANSocket {
        -int fd_
        -string interface_name_
        -uint32_t timeout_ms_
        -bool is_open_
        +RealCANSocket(interface, timeout)
        +send(frame) ssize_t
        +receive(frame) ssize_t
        +get_fd() int
        +is_open() bool
        +close() void
        +get_interface_name() string
        -open_socket() void
        -configure_socket() void
    }
    
    class MockCANSocket {
        -queue~can_frame~ tx_queue_
        -queue~can_frame~ rx_queue_
        -bool open_
        -int fake_fd_
        +send(frame) ssize_t
        +receive(frame) ssize_t
        +push_rx_frame(frame) void
        +pop_tx_frame() can_frame
        +get_fd() int
        +is_open() bool
    }
    
    %% ===================================================================
    %% USB Adapter (Uses ISerialPort)
    %% ===================================================================
    
    class USBAdapter {
        -unique_ptr~ISerialPort~ serial_port_
        -string usb_device_
        -SerialBaud baudrate_
        -bool is_configured_
        -shared_mutex state_mutex_
        -mutex write_mutex_
        -mutex read_mutex_
        +create(device, baud) unique_ptr~USBAdapter~$
        +USBAdapter(serial_port, device, baud)
        +send_frame~T~(frame) void
        +receive_fixed_frame(timeout) FixedFrame
        +receive_variable_frame(timeout) VariableFrame
        +is_open() bool
        +is_configured() bool
        -write_bytes(data, size) void
        -read_bytes(buffer, size, timeout) ssize_t
    }
    
    %% ===================================================================
    %% Configuration Enums
    %% ===================================================================
    
    class SerialBaud {
        <<enumeration>>
        BAUD_9600
        BAUD_19200
        BAUD_38400
        BAUD_57600
        BAUD_115200
        BAUD_2M
    }
    
    %% ===================================================================
    %% Relationships
    %% ===================================================================
    
    ISerialPort <|.. RealSerialPort : implements
    ISerialPort <|.. MockSerialPort : implements
    ICANSocket <|.. RealCANSocket : implements
    ICANSocket <|.. MockCANSocket : implements
    
    USBAdapter o-- ISerialPort : owns
    USBAdapter ..> SerialBaud : uses
    
    RealSerialPort ..> SerialBaud : uses
```

## Design Pattern: Dependency Injection

### Interface-Based Abstraction
- **ISerialPort**: Abstract serial communication
- **ICANSocket**: Abstract CAN socket operations
- **Benefit**: Swap implementations without changing client code

### Production Implementations
- **RealSerialPort**: Linux termios2-based serial I/O
- **RealCANSocket**: Linux SocketCAN (AF_CAN sockets)
- **Features**: Proper error handling, configurable timeouts

### Test Implementations
- **MockSerialPort**: Queue-based simulation
- **MockCANSocket**: Queue-based simulation
- **Features**: Inject test data, verify transmitted data

## USBAdapter: Thread-Safe Serial Communication

### Three-Mutex Pattern
1. **state_mutex_ (shared_mutex)**: Protects configuration state (read-heavy)
2. **write_mutex_**: Serializes write operations
3. **read_mutex_**: Serializes read operations (for multi-threaded scenarios)

### Responsibilities
- Frame serialization/deserialization
- Byte-level protocol handling
- Timeout management
- Error recovery

## Platform Support

### Linux (Primary)
- **Serial**: `/dev/ttyUSBx` via termios2
- **CAN**: SocketCAN (vcan, can0, can1, etc.)
- **Features**: Non-blocking I/O, custom baud rates

### Testing (Any Platform)
- **Mock**: No hardware required
- **Queues**: Inject/verify data in unit tests
- **CI/CD**: Runs without physical devices

## Usage Examples

### Production (Real Hardware)
```cpp
// Create real serial port
auto serial = std::make_unique<RealSerialPort>("/dev/ttyUSB0", SerialBaud::BAUD_2M);

// Create USB adapter
auto adapter = std::make_unique<USBAdapter>(std::move(serial), "/dev/ttyUSB0", SerialBaud::BAUD_2M);

// Send frame
VariableFrame frame = ...;
adapter->send_frame(frame);
```

### Testing (Mock Hardware)
```cpp
// Create mock serial port
auto mock_serial = std::make_unique<MockSerialPort>();

// Inject response data
mock_serial->push_rx_data({0xAA, 0x55, 0x12, 0x34});

// Create adapter with mock
auto adapter = std::make_unique<USBAdapter>(std::move(mock_serial), "mock", SerialBaud::BAUD_2M);

// Send frame (captured in mock)
adapter->send_frame(frame);

// Verify transmitted bytes
auto tx_data = mock_serial->pop_tx_data();
```

### CANopen (Direct Socket Access)
```cpp
// Create real CAN socket
auto socket = std::make_shared<RealCANSocket>("vcan0", 1000);

// Use with SDOClient
SDOClient sdo(socket, dictionary, node_id);
uint16_t statusword = sdo.read<uint16_t>("statusword");
```

## Error Handling

### Exceptions
- **DeviceException**: Device not found, permission denied
- **TimeoutException**: Read/write timeout expired
- **ProtocolException**: Invalid frame, checksum error

### Timeout Behavior
- **Blocking**: read() blocks until data or timeout
- **Non-blocking**: Can be configured with timeout=0
- **Retry**: Application decides retry logic
