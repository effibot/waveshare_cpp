# Frame System - Class Diagram

The frame system implements the Waveshare USB-CAN protocol with a state-first, CRTP-based design.

```mermaid

---
config:
  theme: forest
---

classDiagram
    %% ===================================================================
    %% Core State Objects
    %% ===================================================================
    
    class CoreState {
        <<struct>>
        +Type type
        +CANVersion can_version
    }
    
    class DataState {
        <<struct>>
        +Format format
        +uint32_t can_id
        +uint8_t dlc
        +vector~uint8_t~ data
    }
    
    class ConfigState {
        <<struct>>
        +CANBaud baud_rate
        +CANMode can_mode
        +RTX auto_rtx
        +uint32_t filter
        +uint32_t mask
    }
    
    %% ===================================================================
    %% CRTP Base Interfaces
    %% ===================================================================
    
    class CoreInterface~Frame~ {
        <<interface>>
        #CoreState core_state_
        +Type get_type()
        +CANVersion get_can_version()
        +vector~uint8_t~ serialize()
        +void deserialize(span)
        +size_t serialized_size()
        #Frame& derived()*
        #vector~uint8_t~ impl_serialize()*
        #void impl_deserialize(span)*
    }
    
    class DataInterface~Frame~ {
        <<interface>>
        #DataState data_state_
        +uint32_t get_id()
        +uint8_t get_dlc()
        +span~uint8_t~ get_data()
        +void set_data(span)
        +Format get_format()
        #bool impl_is_extended()*
    }
    
    class ConfigInterface~Frame~ {
        <<interface>>
        #ConfigState config_state_
        +CANBaud get_baud_rate()
        +CANMode get_can_mode()
        +RTX get_auto_rtx()
        +uint32_t get_filter()
        +uint32_t get_mask()
    }
    
    %% ===================================================================
    %% Concrete Frame Classes
    %% ===================================================================
    
    class FixedFrame {
        +FixedFrame()
        +FixedFrame(Format, CANVersion, id, data)
        +vector~uint8_t~ impl_serialize()
        +void impl_deserialize(span)
        +size_t impl_serialized_size()
        +bool impl_is_extended()
    }
    
    class VariableFrame {
        +VariableFrame()
        +VariableFrame(Format, CANVersion, id, data)
        +vector~uint8_t~ impl_serialize()
        +void impl_deserialize(span)
        +size_t impl_serialized_size()
        +bool impl_is_extended()
    }
    
    class ConfigFrame {
        +ConfigFrame()
        +ConfigFrame(Type, baud, mode, rtx, filter, mask, version)
        +vector~uint8_t~ impl_serialize()
        +void impl_deserialize(span)
        +size_t impl_serialized_size()
    }
    
    %% ===================================================================
    %% Builder Pattern
    %% ===================================================================
    
    class FrameBuilder~Frame~ {
        -FrameBuilderState~Frame~ state_
        +FrameBuilder& with_type(Type)
        +FrameBuilder& with_format(Format)
        +FrameBuilder& with_id(uint32_t)
        +FrameBuilder& with_data(span)
        +FrameBuilder& with_baud_rate(CANBaud)
        +FrameBuilder& finalize()
        +Frame build()
    }
    
    %% ===================================================================
    %% Helper Utilities
    %% ===================================================================
    
    class ChecksumHelper {
        <<utility>>
        +uint8_t compute(buffer)$
        +bool validate(buffer)$
    }
    
    class VarTypeHelper {
        <<utility>>
        +uint8_t encode(is_ext, format, dlc)$
        +tuple decode(type_byte)$
    }
    
    class SocketCANHelper {
        <<utility>>
        +can_frame to_socketcan(VariableFrame)$
        +VariableFrame from_socketcan(can_frame)$
    }
    
    %% ===================================================================
    %% Relationships
    %% ===================================================================
    
    CoreInterface~Frame~ *-- CoreState
    DataInterface~Frame~ *-- DataState
    ConfigInterface~Frame~ *-- ConfigState
    
    CoreInterface~FixedFrame~ <|-- DataInterface~FixedFrame~
    DataInterface~FixedFrame~ <|-- FixedFrame
    
    CoreInterface~VariableFrame~ <|-- DataInterface~VariableFrame~
    DataInterface~VariableFrame~ <|-- VariableFrame
    
    CoreInterface~ConfigFrame~ <|-- ConfigInterface~ConfigFrame~
    ConfigInterface~ConfigFrame~ <|-- ConfigFrame
    
    FrameBuilder~Frame~ ..> FixedFrame : creates
    FrameBuilder~Frame~ ..> VariableFrame : creates
    FrameBuilder~Frame~ ..> ConfigFrame : creates
    
    FixedFrame ..> ChecksumHelper : uses
    ConfigFrame ..> ChecksumHelper : uses
    VariableFrame ..> VarTypeHelper : uses
    SocketCANHelper ..> VariableFrame : converts
```

## Design Patterns

### CRTP (Curiously Recurring Template Pattern)
- **Purpose**: Zero-overhead compile-time polymorphism
- **Implementation**: Base classes call derived() to access frame-specific methods
- **Benefit**: No virtual function overhead, all resolved at compile time

### State-First Architecture
- **CoreState**: Common properties (type, CAN version)
- **DataState**: Data frame properties (ID, DLC, data)
- **ConfigState**: Configuration properties (baud, mode, filters)
- **Benefit**: Clear separation, efficient memory layout

### Builder Pattern
- **Fluent Interface**: Chain method calls for frame construction
- **Validation**: Enforces required fields before build()
- **Type Safety**: Compile-time type checking

## Frame Types

### FixedFrame (20 bytes)
- Fixed-size protocol frame
- Always includes checksum
- Layout: `START | HEADER | TYPE | VERS | FMT | ID(4) | DLC | DATA(8) | RES | CHK`
- **Use case**: Legacy protocol, checksummed communication

### VariableFrame (4-14 bytes)
- Variable-size protocol frame
- No checksum (relies on USB layer)
- Layout: `START | TYPE | ID(1-4) | DATA(0-8)`
- **Use case**: Modern protocol, efficient bandwidth

### ConfigFrame (20 bytes)
- Configuration command frame
- Fixed size with checksum
- Layout: `START | HEADER | TYPE | BAUD | VERS | FILTER(4) | MASK(4) | MODE | RTX | RES | CHK`
- **Use case**: Adapter initialization and configuration

## Usage Example

```cpp
// Build a variable data frame
auto frame = FrameBuilder<VariableFrame>()
    .with_format(Format::DATA_VARIABLE)
    .with_id(0x123)
    .with_data({0x11, 0x22, 0x33})
    .finalize()
    .build();

// Serialize to bytes
auto bytes = frame.serialize();

// Convert to SocketCAN
auto can_frame = SocketCANHelper::to_socketcan(frame);
```
