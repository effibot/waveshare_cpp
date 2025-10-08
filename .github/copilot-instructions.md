# Waveshare USB-CAN-A C++ Library - AI Coding Instructions

## Architecture Overview

This library implements a type-safe C++ interface for Waveshare USB-CAN-A device protocol using **CRTP (Curiously Recurring Template Pattern)** and modern C++ patterns.

### Core Design Pattern: CRTP-Based Interface Hierarchy

The codebase uses CRTP extensively for compile-time polymorphism:

```
CoreInterface<Frame>           ← Base for all frames (storage, layout, lifecycle)
├── DataInterface<Frame>       ← For CAN data frames (fixed & variable)
│   ├── FixedFrame            ← 20-byte fixed frames
│   └── VariableFrame         ← 5-15 byte variable frames
└── ConfigInterface<Frame>     ← For USB adapter configuration
    └── ConfigFrame           ← Configuration frames
```

**Key Pattern**: Each frame class derives from an interface that takes itself as template parameter:
- `class FixedFrame : public DataInterface<FixedFrame>`
- `class VariableFrame : public DataInterface<VariableFrame>`
- Interfaces use `derived()` to call frame-specific `impl_*()` methods

### Frame Types & Protocol

Three frame types implement the Waveshare serial protocol:

1. **FixedFrame** (20 bytes): `[START][HEADER][TYPE][CAN_VERS][FORMAT][ID(4)][DLC][DATA(8)][RESERVED][CHECKSUM]`
   - Always 8-byte data field (padded with zeros if DLC < 8)
   - Has checksum validation via `ChecksumInterface<FixedFrame>`

2. **VariableFrame** (5-15 bytes): `[START][TYPE][ID(2/4)][DATA(0-8)][END]`
   - Dynamic size based on ID type (2 or 4 bytes) and DLC (0-8 bytes)
   - TYPE byte encodes: `[0xC0][IsExtended][Format][DLC(4 bits)]`
   - Managed by `VarTypeInterface<VariableFrame>` for TYPE field reconstruction
   - Uses internal buffer splitting/merging for size changes

3. **ConfigFrame** (20 bytes): Configure USB adapter settings (baud rate, mode, filters)

### Critical Implementation Details

#### Impl Methods Pattern
Frame-specific logic lives in `impl_*()` methods called by base interfaces via CRTP:
- `impl_init_fields()`: Initialize constant protocol fields
- `impl_get_type()`, `impl_set_type()`: Type byte access
- `impl_get_CAN_id()`, `impl_set_CAN_id()`: ID manipulation
- `impl_get_dlc()`, `impl_set_dlc()`: Data length code

#### FrameTraits System
`include/template/frame_traits.hpp` provides compile-time frame metadata:
- **StorageType**: `std::array<std::uint8_t, 20>` (fixed) or `boost::span<std::uint8_t>` (variable)
- **Layout**: Nested struct with byte offsets (e.g., `layout_.ID`, `layout_.CHECKSUM`)
- **Type predicates**: `is_data_frame_v<T>`, `is_config_frame_v<T>`, `has_checksum_v<T>`

Access traits via:
```cpp
using storage_t<Frame>  // Get storage type
using layout_t<Frame>   // Get layout type
frame_traits_t<Frame>   // Full traits
```

#### Dirty Bit Pattern
Both checksum and TYPE field use dirty tracking:
- `ChecksumInterface`: Calls `mark_dirty()` after frame modifications, `mark_clean()` after recomputation
- `VarTypeInterface`: Tracks when TYPE byte needs reconstruction from CAN_VERS/Format/DLC

#### Result Type
`Result<T>` in `include/template/result.hpp` wraps values or `Status` errors with **automatic error chaining**:
```cpp
Result<bool> validate() {
    auto res = validator.check();
    if (res.fail()) return res;  // Chains error context automatically
    return Result<bool>::success(true);
}
```
- Use `.ok()`, `.fail()`, `.value()`, `.error()` 
- Call `.describe()` for full error chain with context

### Builder Pattern
`FrameBuilder<Frame>` provides fluent, validated construction:
```cpp
auto frame = FrameBuilder<FixedFrame>()
    .with_can_version(CANVersion::STD_FIXED)
    .with_format(Format::DATA_FIXED)
    .with_id(0x123)
    .with_data({0x01, 0x02})
    .finalize()
    .build();
```
- SFINAE restricts methods to appropriate frame types
- `validate_state()` checks required fields before `build()`

### Validation System
Validators in `include/interface/validator/` provide protocol compliance:
- `CoreValidator<Frame>`: Validates START, TYPE, CAN_VERS, Format bytes
- `DataValidator<Frame>`: Validates ID ranges, DLC, data payload
- `ConfigValidator<Frame>`: Validates baud rates, modes, filters/masks

Return `Result<bool>` with specific `Status` errors.

## Important Conventions

### ID & Endianness
- CAN IDs stored in **little-endian** in protocol bytes: ID `0x123` → `[0x23][0x01]`
- Standard ID: 11 bits (max `0x7FF`), Extended ID: 29 bits (max `0x1FFFFFFF`)
- Use `layout_.id_size(is_extended)` for dynamic ID size in VariableFrame

### DLC vs Data Size
- **DLC** (Data Length Code): Number of **valid** data bytes (0-8)
- **FixedFrame**: DATA field always 8 bytes (pad with zeros if DLC < 8)
- **VariableFrame**: DATA field size equals DLC (no padding)

### Type vs CAN_VERS Naming
- Protocol doc uses `FRAME_TYPE` for what library calls `CAN_VERS` (enum class `CANVersion`)
- Library `Type` refers to frame type: `DATA_FIXED`, `DATA_VARIABLE`, `CONF_FIXED`, `CONF_VARIABLE`

### Namespace & Dependencies
- All code in `namespace USBCANBridge`
- Uses `boost::span` for view semantics (aliased via `using namespace boost`)
- C++17 minimum (uses `std::uint8_t` for byte storage, `if constexpr`, `std::optional`)

## Code Formatting
- Uncrustify config in `uncrustify.cfg` (4-space indentation, K&R-like style)
- Comment-based impl sections: `// === Section Name ===` or `// * Comment`

## Key Files Reference
- `include/enums/protocol.hpp`: All protocol constants and enum definitions
- `include/template/frame_traits.hpp`: Compile-time frame metadata system
- `include/template/result.hpp`: Error handling with automatic chaining
- `include/pattern/frame_builder.hpp`: Fluent builder pattern
- `include/interface/core.hpp`: Base CRTP interface with storage/layout access
- `README.md`: Complete protocol documentation with diagrams in `doc/diagrams/`

## When Working With This Codebase

1. **Adding new frame operations**: Add `impl_*()` method to frame class, expose via interface
2. **Modifying frame structure**: Update `FrameTraits` specialization and `Layout` struct
3. **New validation**: Add to appropriate validator, return `Result<bool>` with specific `Status`
4. **Frame modifications**: Call `mark_dirty()` on checksum/TYPE interfaces after changes
5. **Error propagation**: Use `Result<T>` return types, chain errors via factory methods
