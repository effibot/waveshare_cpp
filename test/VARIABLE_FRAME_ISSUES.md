# Remaining Issues - VariableFrame

## Critical Bug: Initialization Order and Buffer Management

### Problem
VariableFrame has a critical bug where the CAN ID and data are not being properly initialized in the constructor. This is causing:

1. **Wrong ID values**: `get_can_id()` returns values like `0x5500` (START/HEADER bytes) or `0xbbaa2301` (garbage + expected bytes)
2. **Memory corruption**: "Payload size exceeds maximum" exceptions and crashes (SIGABRT, "free(): invalid pointer")

### Root Cause
The issue is a complex interaction between:

1. **Initialization order**: Base class `DataInterface` constructor calls `impl_init_fields()` BEFORE member variables (`current_dlc_`, `init_id`, etc.) are fully initialized
2. **Buffer resizing**: `impl_init_fields()` resizes the `traits_.frame_buffer` vector and updates the `frame_storage_` span
3. **Constructor body**: Tries to copy ID/data AFTER base initialization, but the buffer/span state may be inconsistent

### Current Constructor Flow
```cpp
VariableFrame(...) 
    : DataInterface<VariableFrame>(),  // 1. Base init
      current_format_(fmt),             // 2. Member init
      current_version_(ver),
      current_dlc_(payload_size),
      init_id(init_id_param),
      init_data(init_data_param),
      var_type_interface_(*this) {
    
    // 3. Base constructor called impl_init_fields() which:
    //    - Resized buffer based on current_dlc_ and current_version_
    //    - Updated frame_storage_ span
    //    - Set TYPE and END bytes
    
    // 4. Constructor body tries to copy ID/data:
    if (!init_id_param.empty()) {
        auto id_span = this->get_storage().subspan(layout_.ID, init_id_param.size());
        std::copy(init_id_param.begin(), init_id_param.end(), id_span.begin());
    }
}
```

### Issues with Current Approach

1. **Buffer state**: After `impl_init_fields()` resizes the buffer, the `frame_storage_` span is updated, but there may be timing issues with when the span is valid
2. **Subspan calculation**: `get_storage().subspan(layout_.ID, init_id_param.size())` might be returning a span that doesn't match the actual buffer layout
3. **Uninitialized bytes**: Resizing with `resize(new_size, 0x00)` should initialize new bytes, but something is going wrong

### Debugging Output Needed

To fix this, we need to:

1. Print the actual buffer contents after each step:
   - After CoreInterface init
   - After impl_init_fields() resize
   - After ID copy
   - After data copy

2. Verify span validity:
   - Check `frame_storage_.data()` pointer
   - Check `frame_storage_.size()`
   - Check `traits_.frame_buffer.data()` pointer
   - Verify they match

3. Check `get_CAN_id_span()` behavior:
   - What offset does `layout_.ID` have?
   - What size does `get_id_field_size()` return?
   - Does the subspan match the actual ID location?

### Potential Solutions

#### Option 1: Delay ID/Data Initialization
Don't initialize ID/data in constructor - require explicit `set_id()` and `set_data()` calls:
```cpp
VariableFrame frame(Format::DATA_VARIABLE, CANVersion::STD_VARIABLE);
frame.set_id(0x123);
frame.set_data({0xAA, 0xBB, 0xCC});
```

#### Option 2: Two-Phase Initialization
Split initialization into setup phase and data phase:
```cpp
void impl_init_fields() {
    // Only resize and set constant fields
    resize_buffer();
    set_constant_fields();
}

void apply_constructor_params(id, data) {
    // Called from constructor body AFTER all init
    copy_id_if_provided();
    copy_data_if_provided();
}
```

#### Option 3: Use FrameBuilder
Always use `FrameBuilder<VariableFrame>` to construct frames with data:
```cpp
auto frame = FrameBuilder<VariableFrame>()
    .with_format(Format::DATA_VARIABLE)
    .with_version(CANVersion::STD_VARIABLE)
    .with_id({0x01, 0x23})
    .with_data({0xAA, 0xBB, 0xCC, 0xDD})
    .build();
```

## Test Status

### ✅ Passing Tests
- **ConfigFrame**: All 16 test cases passing (87 assertions)
- **FixedFrame**: All 15 test cases passing (93 assertions)

### ❌ Failing Tests  
- **VariableFrame**: 12 out of 14 test cases failing
  - 2 passing (basic functionality without ID/data)
  - 3 failures: Wrong CAN ID values
  - 8 failures: "Payload size exceeds maximum" exceptions
  - 1 failure: Memory corruption (SIGABRT)

## Next Steps

1. **Debug buffer state**: Add logging to trace buffer pointer/size through initialization
2. **Verify span consistency**: Ensure `frame_storage_` always points to valid buffer after resize
3. **Test ID span calculation**: Verify `get_CAN_id_span()` returns correct offset and size
4. **Consider alternative approach**: Maybe VariableFrame needs different initialization strategy than FixedFrame due to dynamic sizing

## Files to Investigate

- `include/frame/variable_frame.hpp`: Constructor and `impl_init_fields()`
- `include/interface/core.hpp`: Base initialization and buffer setup
- `include/interface/data.hpp`: `get_CAN_id_span()` and ID access methods
- `src/variable_frame.cpp`: `impl_get_CAN_id()` and `impl_set_CAN_id()`
