# Prompt 3: Implementation with Code Context

Here's the current USBAdapter API that WaveshareSocketCANBridge will use:

## USBAdapter API

```cpp
// USBAdapter private methods (accessible via friend declaration)
Result<int> write_bytes(const std::uint8_t* data, std::size_t size);
Result<int> read_bytes(std::uint8_t* buffer, std::size_t size);
Result<void> read_exact(std::uint8_t* buffer, std::size_t size, int timeout_ms);

// USBAdapter public frame API
template<typename Frame>
Result<void> send_frame(const Frame& frame);

template<typename Frame>
Result<Frame> receive_fixed_frame(int timeout_ms = 1000);

Result<VariableFrame> receive_variable_frame(int timeout_ms = 1000);
```

## Implementation Requirements

Implement the WaveshareSocketCANBridge class with:

### 1. Constructor
- Open USB adapter
- Create SocketCAN socket

### 2. configure()
- Build ConfigFrame with BridgeConfig settings (baud, mode, filters)
- Send via adapter_->write_bytes()
- Read acknowledgment using adapter_->read_exact() with timeout
- Validate ACK matches sent config

### 3. start()
- Spawn threads calling usb_to_socketcan_loop() / socketcan_to_usb_loop()

### 4. usb_to_socketcan_loop() pseudocode

```cpp
while(running_):
    bytes = read_variable_frame_bytes(timeout)  // 0xAA...0x55 detection
    frame = VariableFrame::deserialize(bytes)
    can_frame = frame_to_socketcan(frame)
    write(socketcan_fd_, &can_frame)
```

### 5. socketcan_to_usb_loop() pseudocode

```cpp
while(running_):
    read(socketcan_fd_, &can_frame)
    varframe = socketcan_to_frame(can_frame)  // FrameBuilder pattern
    buffer = varframe.serialize()
    adapter_->write_bytes(buffer)
```

## Deliverables

Please provide:
- Complete header file (include/bridge/socketcan_bridge.hpp)
- Implementation file (src/socketcan_bridge.cpp)
- Follow State-First pattern (frames transient, no persistent buffers)
- Use Result<T> for all error paths
- Add statistics tracking (Statistics struct with atomic counters)
