# Waveshare USB-CAN Library - Architecture Documentation

> **✨ This library has been restructured into modular subsystem diagrams for better clarity.**

## 📚 Modular Architecture Diagrams

The architecture is divided into clear subsystems. Navigate to the appropriate diagram for your needs:

### 1. [Architecture Overview](00_architecture_overview.md)
High-level system structure showing 6 layers:
- Hardware Layer (USB adapter, SocketCAN)
- I/O Abstraction Layer
- Frame Layer
- Bridge Layer
- Protocol Layer (CANopen)
- Application Layer

### 2. [Frame System](01_frame_system.md)
Frame classes and the CRTP builder pattern:
- `FixedFrame`, `VariableFrame`, `ConfigFrame`
- `FrameBuilder` with type-safe construction
- Frame validation and serialization

### 3. [I/O Abstraction](02_io_abstraction.md)
Serial and CAN interfaces with dependency injection:
- `ISerialPort` / `RealSerialPort`
- `ICANSocket` / `RealCANSocket`
- Mock implementations for testing

### 4. [Bridge System](03_bridge_system.md)
SocketCANBridge dual-thread architecture:
- Bidirectional frame forwarding
- Lock-free atomic statistics
- Performance monitoring

### 5. [CANopen Protocol Stack](04_canopen_system.md) 🆕
Complete CANopen implementation:
- `SDOClient` - Service Data Objects
- `PDOManager` - Process Data Objects  
- `CIA402FSM` - Motor control state machine
- `ObjectDictionary` - EDS/JSON configuration

---

## Design Principles

The library follows these architectural principles:

- **Dependency Injection**: All I/O dependencies are injected for testability
- **CRTP Pattern**: Type-safe frame building without virtual overhead
- **Lock-Free Concurrency**: Atomic operations for performance monitoring
- **Clear Layering**: Each layer has well-defined responsibilities
- **Protocol Agnostic**: Core components work with any CAN protocol

---

## Quick Start

```cpp
// 1. Create I/O abstractions
auto serial = std::make_unique<RealSerialPort>("/dev/ttyUSB0", 2000000);
auto socket = std::make_unique<RealCANSocket>("vcan0", 1000);

// 2. Create bridge
BridgeConfig config;
config.can_interface = "vcan0";
auto bridge = std::make_unique<SocketCANBridge>(
    config, std::move(socket), std::move(serial));

// 3. Use CANopen
auto canopen_socket = std::make_shared<RealCANSocket>("vcan0", 1000);
ObjectDictionary dict("motor_config.json");
SDOClient sdo(canopen_socket, dict, 1);
uint16_t statusword = sdo.read<uint16_t>("statusword");
```

---

For historical reference, the old monolithic class diagram has been archived.  
**Use the modular diagrams above for current development.**
