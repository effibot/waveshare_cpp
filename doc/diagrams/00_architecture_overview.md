# Waveshare Library - Architecture Overview

High-level view of the library subsystems and their relationships.

```mermaid

---
config:
  theme: forest
---

graph TB
    subgraph "Application Layer"
        APP[Application Code]
    end
    
    subgraph "CANopen Protocol Layer"
        CANOPEN[CANopen Stack<br/>SDO/PDO/CIA402]
    end
    
    subgraph "Bridge Layer"
        BRIDGE[SocketCAN Bridge<br/>USB ↔ SocketCAN]
    end
    
    subgraph "Protocol Layer"
        FRAMES[Frame System<br/>Fixed/Variable/Config]
        ADAPTER[USB Adapter<br/>Serial Communication]
    end
    
    subgraph "I/O Abstraction Layer"
        SERIAL[ISerialPort<br/>RealSerialPort]
        CAN[ICANSocket<br/>RealCANSocket]
    end
    
    subgraph "Hardware Layer"
        USB[Waveshare USB-CAN<br/>Hardware]
        SOCKETCAN[Linux SocketCAN<br/>vcan/can0]
    end
    
    APP --> CANOPEN
    APP --> BRIDGE
    CANOPEN --> CAN
    BRIDGE --> ADAPTER
    BRIDGE --> CAN
    ADAPTER --> FRAMES
    ADAPTER --> SERIAL
    SERIAL --> USB
    CAN --> SOCKETCAN
    
    style CANOPEN fill:#4a9eff,stroke:#2980b9,color:#fff
    style BRIDGE fill:#2ecc71,stroke:#27ae60,color:#fff
    style FRAMES fill:#e74c3c,stroke:#c0392b,color:#fff
    style ADAPTER fill:#e74c3c,stroke:#c0392b,color:#fff
    style SERIAL fill:#f39c12,stroke:#d68910,color:#fff
    style CAN fill:#f39c12,stroke:#d68910,color:#fff
    style USB fill:#95a5a6,stroke:#7f8c8d,color:#fff
    style SOCKETCAN fill:#95a5a6,stroke:#7f8c8d,color:#fff
```

## Subsystems

### 1. **CANopen Protocol Layer** 🆕
- **Purpose**: High-level CANopen communication (CIA301/CIA402)
- **Components**: SDOClient, PDOManager, CIA402FSM, ObjectDictionary
- **Dependencies**: ICANSocket (uses SocketCAN directly)
- **Key Feature**: Motor control and industrial automation protocols

### 2. **Bridge Layer**
- **Purpose**: Bidirectional frame forwarding between USB and SocketCAN
- **Components**: SocketCANBridge, BridgeConfig, BridgeStatistics
- **Threading**: Dual-thread architecture for concurrent forwarding
- **Key Feature**: Zero-copy, lock-free performance monitoring

### 3. **Protocol Layer**
- **Purpose**: Waveshare USB-CAN protocol implementation
- **Components**: Frame classes (Fixed/Variable/Config), FrameBuilder, USBAdapter
- **Pattern**: CRTP for zero-overhead abstraction
- **Key Feature**: State-first design with on-demand serialization

### 4. **I/O Abstraction Layer**
- **Purpose**: Hardware abstraction for dependency injection
- **Components**: ISerialPort, ICANSocket, Real* implementations
- **Pattern**: Interface-based design
- **Key Feature**: Enables testing without hardware (Mock implementations)

### 5. **Hardware Layer**
- **Components**: Waveshare USB-CAN adapter, Linux SocketCAN interfaces
- **Access**: Through abstraction layer only
- **Key Feature**: Platform-specific, hidden from higher layers

## Data Flow Examples

### CANopen Motor Control
```
Application → SDOClient → RealCANSocket → vcan0/can0 → Motor
Motor → vcan0/can0 → PDOManager (callbacks) → Application
```

### Bridge Mode
```
Waveshare USB → USBAdapter → VariableFrame → SocketCANBridge → can_frame → vcan0
vcan0 → can_frame → SocketCANBridge → VariableFrame → USBAdapter → Waveshare USB
```

### Direct USB Access
```
Application → FrameBuilder → VariableFrame → USBAdapter → RealSerialPort → USB
USB → RealSerialPort → USBAdapter → VariableFrame → Application
```

## Design Principles

1. **Separation of Concerns**: Each layer has a single, well-defined responsibility
2. **Dependency Injection**: Interfaces enable testing and flexibility
3. **Zero-Copy Where Possible**: Minimize data copying in hot paths
4. **Thread Safety**: Lock-free atomics and careful mutex usage
5. **RAII**: Resource management through constructors/destructors
6. **Compile-Time Optimization**: CRTP and templates for zero overhead
