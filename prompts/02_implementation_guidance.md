# Prompt 2: Request Implementation Guidance

I need to implement the WaveshareSocketCANBridge class with these requirements:

## Design Constraints

1. State-First alignment: Frames created on-demand, destroyed after conversion
2. Thread-safe bidirectional forwarding
3. SocketCAN socket management (PF_CAN, SOCK_RAW, CAN_RAW)
4. VariableFrame byte-by-byte reading (START=0xAA, END=0x55)
5. Configuration handshake: send ConfigFrame, wait for echo acknowledgment

## Class Structure Needed

```cpp
class WaveshareSocketCANBridge {
    friend class USBAdapter;  // For adapter_ access
    
    BridgeConfig config_;
    std::shared_ptr<USBAdapter> adapter_;
    int socketcan_fd_;
    std::atomic<bool> running_;
    std::atomic<bool> configured_;
    std::thread usb_to_socketcan_thread_;
    std::thread socketcan_to_usb_thread_;
    
public:
    Result<void> configure();  // Send ConfigFrame, wait for ACK
    Result<void> start();      // Spawn threads
    Result<void> stop();       // Join threads
};
```

## Key Implementation Questions

1. How should read_variable_frame_bytes() handle timeout during byte-by-byte read?
2. Should frame_to_socketcan() / socketcan_to_frame() be static helpers or member methods?
3. How to handle partial reads in usb_to_socketcan_loop() without blocking?

**Please provide the header file (include/bridge/socketcan_bridge.hpp) with detailed documentation following the coding instructions from `.github/copilot-instructions.md`**
