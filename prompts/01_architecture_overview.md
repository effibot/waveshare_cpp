# Prompt 1: Architecture & Current State Overview

I'm working on a C++ library for the Waveshare USB-CAN-A device that implements a State-First architecture. The library is at this stage:

## Completed Components

1. ✅ Core frame types (FixedFrame, VariableFrame, ConfigFrame) with CRTP interfaces
2. ✅ State-First pattern: frames store runtime state, generate protocol buffers on-demand via serialize()
3. ✅ FrameBuilder pattern with SFINAE-restricted methods for type-safe construction
4. ✅ Result<T> error handling with automatic call stack chaining
5. ✅ FrameTraits system for compile-time layout offsets
6. ✅ Thread-safe USBAdapter class with mutex-protected read/write operations
7. ✅ Full test suite (68 passing tests) using Catch2

## Current Implementation Status

**USBAdapter provides:**
- **Private:** write_bytes(), read_bytes(), read_exact(), flush_port() (raw I/O)
- **Public:** send_frame<Frame>(), receive_fixed_frame<Frame>(), receive_variable_frame()
- **Thread-safety:** std::shared_mutex for state, separate mutexes for read/write
- **Friend access** granted to WaveshareSocketCANBridge for low-level I/O

## Next Phase: SocketCAN Bridge Implementation

We need to implement WaveshareSocketCANBridge that:

1. Opens USB adapter (/dev/ttyCANBUS0) and configures it via ConfigFrame
2. Creates bidirectional bridge: USB serial ↔ SocketCAN (vcan0/can0)
3. Spawns two threads:
   - usb_to_socketcan_loop(): Reads Waveshare frames → writes struct can_frame
   - socketcan_to_usb_loop(): Reads struct can_frame → writes Waveshare frames
4. Handles VariableFrame protocol (0xAA...0x55 framing) byte-by-byte
5. Provides statistics tracking (rx/tx counts, errors)

## Architecture

- Bridge uses friend access to USBAdapter::read_bytes()/write_bytes()
- ROS2 nodes will interact with SocketCAN layer (not directly with bridge)
- Linux-only design leveraging kernel CAN routing

**Please review this context and confirm understanding before I share implementation details.**
