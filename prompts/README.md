# Context Files Reference

When starting fresh on another PC, attach these key files to provide full context:

## 1. Architecture & Patterns

- `.github/copilot-instructions.md` - Coding guidelines and State-First architecture principles
- `README.md` - Protocol documentation and field descriptions
- `doc/diagrams/class_diagram.md` - Class hierarchy overview

## 2. Core Interfaces

- `include/interface/core.hpp` - CoreInterface with CoreState and CRTP
- `include/interface/data.hpp` - DataInterface with DataState (ID, DLC, data)
- `include/template/result.hpp` - Result<T> with automatic error chaining
- `include/template/frame_traits.hpp` - Compile-time layout offsets and type predicates

## 3. Frame Implementations

- `include/frame/fixed_frame.hpp` - 20-byte fixed frames
- `include/frame/variable_frame.hpp` - 5-15 byte variable frames
- `include/frame/config_frame.hpp` - USB config frames

## 4. Current USBAdapter

- `include/pattern/usb_adapter.hpp` - Thread-safe adapter with frame API
- `src/usb_adapter.cpp` - Implementation with mutex-protected I/O

## 5. Build System

- `CMakeLists.txt` - Root build configuration
- `test/CMakeLists.txt` - Test discovery with Catch2

## 6. Example Test

- `test/test_fixed_frame.cpp` - Reference for test structure and patterns

## 7. Protocol Documentation

- `doc/diagrams/waveshare_fixed_data_packet.md` - Fixed frame protocol
- `doc/diagrams/waveshare_variable_data_packet.md` - Variable frame protocol
- `doc/diagrams/waveshare_config_packet.md` - Config frame protocol

## Usage

These files provide the AI with full context on:
- State-First architecture principles
- CRTP interface patterns
- Error handling with Result<T>
- Testing approach with Catch2
- Protocol serialization/deserialization
- Thread-safety patterns

Simply attach these files when starting a new chat to continue development seamlessly.
