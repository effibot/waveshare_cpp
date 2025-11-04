# Waveshare USB-CAN Library - Class Diagram

This diagram shows the complete class hierarchy and relationships in the Waveshare USB-CAN C++ library, including frame classes, interfaces, adapters, and bridge components.

```mermaid

---
config:
  theme: "dark"
  themeVariables: {
    "primaryColor": "#C5D6DE",
    "primaryTextColor": "#1a1a1a",
    "primaryBorderColor": "#6b8a96",
    "lineColor": "#703B3B",
    "secondaryColor": "#EBE3D3",
    "tertiaryColor": "#B9A788",
    "mainBkg": "#C5D6DE",
    "secondBkg": "#EBE3D3",
    "tertiaryBkg": "#B9A788",
    "mainContrastColor": "#ffffff",
    "darkMode": false,
    "background": "#ffffff",
    "fontSize": "16px",
    "fontFamily": "\"Segoe UI\", \"Roboto\", \"Helvetica Neue\", sans-serif",
    "nodeBorder": "#703B3B",
    "clusterBkg": "#EBE3D3",
    "clusterBorder": "#A18D6D",
    "defaultLinkColor": "#703B3B",
    "titleColor": "#703B3B",
    "edgeLabelBackground": "#ffffff",
    "actorBorder": "#703B3B",
    "actorBkg": "#C5D6DE",
    "actorTextColor": "#1a1a1a",
    "actorLineColor": "#703B3B",
    "signalColor": "#1a1a1a",
    "signalTextColor": "#1a1a1a",
    "labelBoxBkgColor": "#EBE3D3",
    "labelBoxBorderColor": "#A18D6D",
    "labelTextColor": "#1a1a1a",
    "loopTextColor": "#1a1a1a",
    "noteBorderColor": "#A18D6D",
    "noteBkgColor": "#f5f5f5",
    "noteTextColor": "#1a1a1a",
    "activationBorderColor": "#703B3B",
    "activationBkgColor": "#EBE3D3",
    "sequenceNumberColor": "#ffffff"
  }
---
classDiagram
    %% ===================================================================
    %% Core State Objects (State-First Architecture)
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
    %% Core Template Interfaces (CRTP Pattern)
    %% ===================================================================
    
    class CoreInterface~Frame~ {
        <<interface>>
        #CoreState core_state_
        +Type get_type()
        +void set_type(Type)
        +CANVersion get_can_version()
        +void set_can_version(CANVersion)
        +vector~uint8_t~ serialize()
        +void deserialize(span)
        +size_t serialized_size()
        +string to_string()
        #Frame& derived()
        #vector~uint8_t~ impl_serialize()*
        #void impl_deserialize(span)*
        #size_t impl_serialized_size()*
    }
    
    class DataInterface~Frame~ {
        <<interface>>
        #DataState data_state_
        +uint32_t get_id()
        +void set_id(uint32_t)
        +uint8_t get_dlc()
        +span~uint8_t~ get_data()
        +void set_data(span)
        +bool is_extended()
        +Format get_format()
        +void set_format(Format)
        #bool impl_is_extended()*
    }
    
    class ConfigInterface~Frame~ {
        <<interface>>
        #ConfigState config_state_
        +CANBaud get_baud_rate()
        +void set_baud_rate(CANBaud)
        +CANMode get_can_mode()
        +void set_can_mode(CANMode)
        +RTX get_auto_rtx()
        +void set_auto_rtx(RTX)
        +uint32_t get_filter()
        +void set_filter(uint32_t)
        +uint32_t get_mask()
        +void set_mask(uint32_t)
    }
    
    %% ===================================================================
    %% Concrete Frame Classes
    %% ===================================================================
    
    class FixedFrame {
        -Traits traits_
        -Layout layout_
        +FixedFrame()
        +FixedFrame(Format, CANVersion, uint32_t, span)
        +vector~uint8_t~ impl_serialize()
        +void impl_deserialize(span)
        +size_t impl_serialized_size()
        +bool impl_is_extended()
    }
    
    class VariableFrame {
        -Traits traits_
        -Layout layout_
        +VariableFrame()
        +VariableFrame(Format, CANVersion, uint32_t, span)
        +vector~uint8_t~ impl_serialize()
        +void impl_deserialize(span)
        +size_t impl_serialized_size()
        +bool impl_is_extended()
    }
    
    class ConfigFrame {
        -Traits traits_
        -Layout layout_
        +ConfigFrame()
        +ConfigFrame(Type, CANBaud, CANMode, RTX, uint32_t, uint32_t, CANVersion)
        +vector~uint8_t~ impl_serialize()
        +void impl_deserialize(span)
        +size_t impl_serialized_size()
    }
    
    %% ===================================================================
    %% Frame Builder Pattern
    %% ===================================================================
    
    class FrameBuilder~Frame~ {
        -FrameBuilderState~Frame~ state_
        +FrameBuilder& with_type(Type)
        +FrameBuilder& with_can_version(CANVersion)
        +FrameBuilder& with_format(Format)
        +FrameBuilder& with_id(uint32_t)
        +FrameBuilder& with_data(span)
        +FrameBuilder& with_baud_rate(CANBaud)
        +FrameBuilder& with_mode(CANMode)
        +FrameBuilder& with_rtx(RTX)
        +FrameBuilder& with_filter(uint32_t)
        +FrameBuilder& with_mask(uint32_t)
        +FrameBuilder& finalize()
        +Frame build()
    }
    
    class FrameBuilderState~Frame~ {
        <<struct>>
        +optional~Type~ type
        +optional~CANVersion~ can_version
        +optional~Format~ format
        +optional~uint32_t~ can_id
        +optional~vector~ data
        +optional~CANBaud~ baud_rate
        +optional~CANMode~ can_mode
        +bool finalized
    }
    
    %% ===================================================================
    %% I/O Abstraction Interfaces (Dependency Injection)
    %% ===================================================================
    
    class ISerialPort {
        <<interface>>
        +ssize_t write(data, len)*
        +ssize_t read(data, len, timeout)*
        +void close()*
        +bool is_open()*
    }
    
    class RealSerialPort {
        -int fd_
        -string device_path_
        -SerialBaud baud_rate_
        -termios2 tty_
        -bool is_open_
        +RealSerialPort(device, baud)
        +ssize_t write(data, len)
        +ssize_t read(data, len, timeout)
        +void close()
        +bool is_open()
        -void open_port()
        -void configure_port()
    }
    
    class ICANSocket {
        <<interface>>
        +int send(can_frame)*
        +can_frame receive(timeout)*
        +int get_fd()*
        +bool is_open()*
        +void close()*
    }
    
    class RealCANSocket {
        -int sockfd_
        -string interface_name_
        -uint32_t read_timeout_ms_
        -bool is_open_
        +RealCANSocket(interface, timeout)
        +int send(can_frame)
        +can_frame receive(timeout)
        +int get_fd()
        +bool is_open()
        +void close()
        -void open_socket()
        -void configure_socket()
    }
    
    %% ===================================================================
    %% USB Adapter (Thread-Safe Serial Communication)
    %% ===================================================================
    
    class USBAdapter {
        -unique_ptr~ISerialPort~ serial_port_
        -string usb_device_
        -SerialBaud baudrate_
        -bool is_configured_
        -shared_mutex state_mutex_
        -mutex write_mutex_
        -mutex read_mutex_
        -static atomic~sig_atomic_t~ stop_flag
        +unique_ptr~USBAdapter~ create(device, baud)$
        +USBAdapter(serial_port, device, baud)
        +void send_frame~T~(frame)
        +FixedFrame receive_fixed_frame(timeout)
        +VariableFrame receive_variable_frame(timeout)
        +bool is_open()
        +bool is_configured()
        +bool should_stop()$
        -void write_bytes(data, size)
        -ssize_t read_bytes(buffer, size, timeout)
        -void read_exact(buffer, size, timeout)
    }
    
    %% ===================================================================
    %% SocketCAN Bridge (Bidirectional Forwarding)
    %% ===================================================================
    
    class BridgeConfig {
        <<struct>>
        +string socketcan_interface
        +string usb_device_path
        +SerialBaud serial_baud_rate
        +CANBaud can_baud_rate
        +CANMode can_mode
        +bool auto_retransmit
        +uint32_t filter_id
        +uint32_t filter_mask
        +uint32_t usb_read_timeout_ms
        +uint32_t socketcan_read_timeout_ms
        +void validate()
        +BridgeConfig create_default()$
        +BridgeConfig from_env()$
        +BridgeConfig from_file(path)$
        +BridgeConfig load(path)$
    }
    
    class BridgeStatistics {
        <<struct>>
        +atomic~uint64_t~ usb_rx_frames
        +atomic~uint64_t~ usb_tx_frames
        +atomic~uint64_t~ socketcan_rx_frames
        +atomic~uint64_t~ socketcan_tx_frames
        +atomic~uint64_t~ usb_rx_errors
        +atomic~uint64_t~ usb_tx_errors
        +atomic~uint64_t~ socketcan_rx_errors
        +atomic~uint64_t~ socketcan_tx_errors
        +atomic~uint64_t~ conversion_errors
        +void reset()
        +string to_string()
    }
    
    class BridgeStatisticsSnapshot {
        <<struct>>
        +uint64_t usb_rx_frames
        +uint64_t usb_tx_frames
        +uint64_t socketcan_rx_frames
        +uint64_t socketcan_tx_frames
        +uint64_t usb_rx_errors
        +uint64_t usb_tx_errors
        +uint64_t socketcan_rx_errors
        +uint64_t socketcan_tx_errors
        +uint64_t conversion_errors
        +string to_string()
    }
    
    class SocketCANBridge {
        -BridgeConfig config_
        -unique_ptr~ICANSocket~ can_socket_
        -unique_ptr~USBAdapter~ adapter_
        -BridgeStatistics stats_
        -atomic~bool~ running_
        -thread usb_to_socketcan_thread_
        -thread socketcan_to_usb_thread_
        -mutex callback_mutex_
        -function callbacks_
        +unique_ptr~SocketCANBridge~ create(config)$
        +SocketCANBridge(config, socket, adapter)
        +void start()
        +void stop()
        +bool is_running()
        +BridgeStatisticsSnapshot get_statistics()
        +void reset_statistics()
        +void set_usb_to_socketcan_callback(fn)
        +void set_socketcan_to_usb_callback(fn)
        +bool is_usb_open()
        +bool is_socketcan_open()
        +int get_socketcan_fd()
        +USBAdapter* get_adapter()
        -void initialize_usb_adapter()
        -void configure_usb_adapter()
        -void verify_adapter_config()
        -void usb_to_socketcan_loop()
        -void socketcan_to_usb_loop()
    }
    
    %% ===================================================================
    %% Helper Classes
    %% ===================================================================
    
    class SocketCANHelper {
        <<utility>>
        +can_frame to_socketcan(VariableFrame)$
        +VariableFrame from_socketcan(can_frame)$
    }
    
    class ChecksumHelper {
        <<utility>>
        +uint8_t compute(buffer, start, end)$
        +bool validate(buffer, pos, start, end)$
    }
    
    class VarTypeHelper {
        <<utility>>
        +uint8_t encode(is_ext, format, dlc)$
        +tuple decode(type_byte)$
    }
    
    %% ===================================================================
    %% Frame Traits System (Compile-Time Metadata)
    %% ===================================================================
    
    class FrameTraits~Frame~ {
        <<trait>>
        +StorageType
        +LayoutType
        +FRAME_SIZE
        +MIN_SIZE
        +MAX_SIZE
        +HAS_CHECKSUM
    }
    
    class FixedFrameLayout {
        <<struct>>
        +START = 0
        +HEADER = 1
        +TYPE = 2
        +CAN_VERS = 3
        +FORMAT = 4
        +ID = 5
        +DLC = 9
        +DATA = 10
        +RESERVED = 18
        +CHECKSUM = 19
    }
    
    class VariableFrameLayout {
        <<struct>>
        +START = 0
        +TYPE = 1
        +ID_START = 2
        +size_t get_data_start(is_ext)$
        +size_t get_end_pos(is_ext, dlc)$
    }
    
    class ConfigFrameLayout {
        <<struct>>
        +START = 0
        +HEADER = 1
        +TYPE = 2
        +CAN_BAUD = 3
        +CAN_VERS = 4
        +FILTER = 5
        +MASK = 9
        +CAN_MODE = 13
        +AUTO_RTX = 14
        +RESERVED = 15
        +CHECKSUM = 19
    }
    
    %% ===================================================================
    %% Exception Hierarchy
    %% ===================================================================
    
    class WaveshareException {
        -Status status_
        +WaveshareException(status, msg)
        +Status status()
        +string what()
    }
    
    class ProtocolException {
        +ProtocolException(status, context)
    }
    
    class DeviceException {
        +DeviceException(status, context)
    }
    
    class TimeoutException {
        +TimeoutException(status, context)
    }
    
    class CANException {
        +CANException(status, context)
    }
    
    %% ===================================================================
    %% Enumerations
    %% ===================================================================
    
    class CANVersion {
        <<enumeration>>
        STD_FIXED
        EXT_FIXED
        STD_VARIABLE
        EXT_VARIABLE
    }
    
    class Type {
        <<enumeration>>
        DATA_FIXED
        DATA_VARIABLE
        CONF_FIXED
        CONF_VARIABLE
        REMOTE_FIXED
        REMOTE_VARIABLE
    }
    
    class Format {
        <<enumeration>>
        DATA_FIXED
        DATA_VARIABLE
        REMOTE_FIXED
        REMOTE_VARIABLE
    }
    
    class CANBaud {
        <<enumeration>>
        BAUD_10K
        BAUD_20K
        BAUD_50K
        BAUD_100K
        BAUD_125K
        BAUD_200K
        BAUD_250K
        BAUD_400K
        BAUD_500K
        BAUD_800K
        BAUD_1M
    }
    
    class CANMode {
        <<enumeration>>
        NORMAL
        LOOPBACK
        SILENT
        LOOPBACK_SILENT
    }
    
    class SerialBaud {
        <<enumeration>>
        BAUD_9600
        BAUD_19200
        BAUD_38400
        BAUD_57600
        BAUD_115200
        BAUD_153600
        BAUD_2M
    }
    
    class RTX {
        <<enumeration>>
        OFF
        AUTO
    }
    
    class Status {
        <<enumeration>>
        SUCCESS
        WBAD_START
        WBAD_CHECKSUM
        WTIMEOUT
        DNOT_FOUND
        DBUSY
        DREAD_ERROR
        DWRITE_ERROR
        ...
    }
    
    %% ===================================================================
    %% State Relationships
    %% ===================================================================
    
    CoreInterface~Frame~ *-- "1" CoreState : contains
    DataInterface~Frame~ *-- "1" DataState : contains
    ConfigInterface~Frame~ *-- "1" ConfigState : contains
    
    %% ===================================================================
    %% CRTP Inheritance (Frame Hierarchy)
    %% ===================================================================
    
    CoreInterface~FixedFrame~ <|-- DataInterface~FixedFrame~ : extends
    DataInterface~FixedFrame~ <|-- FixedFrame : implements
    
    CoreInterface~VariableFrame~ <|-- DataInterface~VariableFrame~ : extends
    DataInterface~VariableFrame~ <|-- VariableFrame : implements
    
    CoreInterface~ConfigFrame~ <|-- ConfigInterface~ConfigFrame~ : extends
    ConfigInterface~ConfigFrame~ <|-- ConfigFrame : implements
    
    %% ===================================================================
    %% I/O Interface Implementations
    %% ===================================================================
    
    ISerialPort <|.. RealSerialPort : implements
    ICANSocket <|.. RealCANSocket : implements
    
    %% ===================================================================
    %% Composition Relationships
    %% ===================================================================
    
    FrameBuilder~Frame~ *-- "1" FrameBuilderState~Frame~ : contains
    FrameBuilder~Frame~ ..> FixedFrame : creates
    FrameBuilder~Frame~ ..> VariableFrame : creates
    FrameBuilder~Frame~ ..> ConfigFrame : creates
    
    FrameTraits~FixedFrame~ *-- "1" FixedFrameLayout : provides
    FrameTraits~VariableFrame~ *-- "1" VariableFrameLayout : provides
    FrameTraits~ConfigFrame~ *-- "1" ConfigFrameLayout : provides
    
    FixedFrame ..> FrameTraits~FixedFrame~ : uses
    VariableFrame ..> FrameTraits~VariableFrame~ : uses
    ConfigFrame ..> FrameTraits~ConfigFrame~ : uses
    
    USBAdapter *-- "1" ISerialPort : owns
    USBAdapter ..> FixedFrame : sends/receives
    USBAdapter ..> VariableFrame : sends/receives
    USBAdapter ..> ConfigFrame : sends
    
    SocketCANBridge *-- "1" BridgeConfig : owns
    SocketCANBridge *-- "1" BridgeStatistics : owns
    SocketCANBridge *-- "1" BridgeStatisticsSnapshot : owns
    SocketCANBridge *-- "1" ICANSocket : owns
    SocketCANBridge *-- "1" USBAdapter : owns
    SocketCANBridge ..> VariableFrame : converts
    SocketCANBridge ..> ConfigFrame : uses
    SocketCANBridge ..> SocketCANHelper : uses
    
    SocketCANHelper ..> VariableFrame : converts
    
    %% ===================================================================
    %% Exception Hierarchy
    %% ===================================================================
    
    std_runtime_error <|-- WaveshareException : extends
    WaveshareException <|-- ProtocolException : extends
    WaveshareException <|-- DeviceException : extends
    WaveshareException <|-- TimeoutException : extends
    WaveshareException <|-- CANException : extends
    
    %% ===================================================================
    %% Enum Usage Relationships
    %% ===================================================================
    
    CoreState ..> Type : uses
    CoreState ..> CANVersion : uses
    DataState ..> Format : uses
    ConfigState ..> CANBaud : uses
    ConfigState ..> CANMode : uses
    ConfigState ..> RTX : uses
    WaveshareException ..> Status : uses
    USBAdapter ..> SerialBaud : uses
    BridgeConfig ..> SerialBaud : uses
    BridgeConfig ..> CANBaud : uses
    BridgeConfig ..> CANMode : uses
```

## Architecture Notes

### State-First Design
- Frame classes store state in CoreState, DataState, ConfigState structs
- Serialization generates protocol buffers on-demand via impl_serialize()
- No persistent buffer storage in frame objects

### CRTP Pattern
- CoreInterface, DataInterface, ConfigInterface use CRTP for compile-time polymorphism
- derived() helper calls frame-specific impl_*() methods
- Type-safe, zero-overhead abstraction

### Dependency Injection
- ISerialPort and ICANSocket interfaces enable testing without hardware
- RealSerialPort and RealCANSocket for production use
- Mock implementations available for unit tests

### Thread Safety
- USBAdapter: Three-mutex pattern (state_mutex_, write_mutex_, read_mutex_)
- SocketCANBridge: Lock-free statistics with atomic operations
- Independent thread paths prevent deadlocks

### Exception Handling
- Exception hierarchy based on std::runtime_error
- Status enum provides error categorization
- Context-rich error messages for debugging
```