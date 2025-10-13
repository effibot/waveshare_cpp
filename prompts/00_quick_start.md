# Quick-Start Prompt (For Immediate Continuation)

I'm continuing development of the Waveshare USB-CAN-A C++ library. Current state:

## Implemented

- Core frame types (FixedFrame, VariableFrame, ConfigFrame) with State-First architecture
- Thread-safe USBAdapter with send_frame()/receive_fixed_frame()/receive_variable_frame()
- Result<T> error handling with call stack chaining
- Full Catch2 test suite (68 passing tests)

## Next Task

Implement WaveshareSocketCANBridge class that:

1. Opens /dev/ttyCANBUS0 and vcan0 SocketCAN interface
2. Sends ConfigFrame to adapter, waits for ACK
3. Spawns bidirectional threads: USB ↔ SocketCAN forwarding
4. Handles VariableFrame framing (0xAA...0x55 detection)
5. Provides statistics (rx/tx counts, errors)

## Design

- Uses friend access to USBAdapter::read_bytes()/write_bytes()
- State-First: frames created on-demand, destroyed after conversion
- Thread-safe with std::atomic<bool> running_ flag

## Request

Provide the complete header file (include/bridge/socketcan_bridge.hpp) following the coding instructions in .github/copilot-instructions.md. Include:

- BridgeConfig struct
- WaveshareSocketCANBridge class with lifecycle methods
- Frame conversion helpers (frame_to_socketcan/socketcan_to_frame)
- Statistics struct with atomic counters
