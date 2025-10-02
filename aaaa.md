/**
 * @file frame_interfaces.hpp
 * @brief Clean interface separation for frame operations
 */

#pragma once
#include "common.hpp"
#include "result.hpp"
#include "frame_traits.hpp"
#include <span>

namespace USBCANBridge {

/**
 * @brief Core interface for all frame types - universal operations
 */
template<typename Derived>
class FrameCore {
public:
    // Universal operations available to ALL frame types
    Result<void> serialize(std::span<std::byte> buffer) const {
        return static_cast<const Derived*>(this)->impl_serialize(buffer);
    }
    
    Result<void> deserialize(std::span<const std::byte> data) {
        return static_cast<Derived*>(this)->impl_deserialize(data);
    }
    
    Result<bool> validate() const {
        return static_cast<const Derived*>(this)->impl_validate();
    }
    
    Result<std::size_t> size() const {
        return static_cast<const Derived*>(this)->impl_size();
    }
    
    Result<void> clear() {
        return static_cast<Derived*>(this)->impl_clear();
    }
    
    std::span<const std::byte> get_raw_data() const {
        return static_cast<const Derived*>(this)->impl_get_raw_data();
    }

protected:
    // Access to derived class
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

/**
 * @brief Interface for frames with CAN ID and payload (FixedFrame, VariableFrame)
 */
template<typename Derived>
class DataFrame : public FrameCore<Derived> {
public:
    // Data frame specific operations - only available for data frames
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<void>>
    set_can_id(uint32_t id) {
        return this->derived().impl_set_can_id(id);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<uint32_t>>
    get_can_id() const {
        return this->derived().impl_get_can_id();
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<void>>
    set_data(std::span<const std::byte> data) {
        return this->derived().impl_set_data(data);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<std::span<const std::byte>>>
    get_data() const {
        return this->derived().impl_get_data();
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<uint8_t>>
    get_dlc() const {
        return this->derived().impl_get_dlc();
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<bool>>
    is_remote() const {
        return this->derived().impl_is_remote();
    }
    
    // FixedFrame specific operations
    template<typename T = Derived>
    std::enable_if_t<is_fixed_frame_v<T>, Result<void>>
    set_frame_type(FrameType type) {
        return this->derived().impl_set_frame_type(type);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_fixed_frame_v<T>, Result<FrameType>>
    get_frame_type() const {
        return this->derived().impl_get_frame_type();
    }
};

/**
 * @brief Interface for configuration operations (ConfigFrame only)
 */
template<typename Derived>
class ConfigInterface : public FrameCore<Derived> {
public:
    // Configuration operations - only available for ConfigFrame
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<void>>
    set_baud_rate(CANBaud rate) {
        return this->derived().impl_set_baud_rate(rate);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<CANBaud>>
    get_baud_rate() const {
        return this->derived().impl_get_baud_rate();
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<void>>
    set_mode(CANMode mode) {
        return this->derived().impl_set_mode(mode);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<CANMode>>
    get_mode() const {
        return this->derived().impl_get_mode();
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<void>>
    set_filter(uint32_t filter) {
        return this->derived().impl_set_filter(filter);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<uint32_t>>
    get_filter() const {
        return this->derived().impl_get_filter();
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<void>>
    set_mask(uint32_t mask) {
        return this->derived().impl_set_mask(mask);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<uint32_t>>
    get_mask() const {
        return this->derived().impl_get_mask();
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<void>>
    set_auto_rtx(RTX auto_rtx) {
        return this->derived().impl_set_auto_rtx(auto_rtx);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<RTX>>
    get_auto_rtx() const {
        return this->derived().impl_get_auto_rtx();
    }
};

} // namespace USBCANBridge

/**
 * @brief Shared checksum functionality for frames that need it
 */

#pragma once
#include "common.hpp"
#include "result.hpp"
#include "frame_traits.hpp"

namespace USBCANBridge {

/**
 * @brief Mixin class providing checksum functionality
 * Only applicable to frames with checksums (FixedFrame, ConfigFrame)
 */
template<typename Derived>
class ChecksumMixin {
public:
    template<typename T = Derived>
    std::enable_if_t<has_checksum_v<T>, Result<uint8_t>>
    calculate_checksum() const {
        return this->derived().impl_calculate_checksum();
    }
    
    template<typename T = Derived>
    std::enable_if_t<has_checksum_v<T>, Result<bool>>
    verify_checksum() const {
        return this->derived().impl_verify_checksum();
    }
    
    template<typename T = Derived>
    std::enable_if_t<has_checksum_v<T>, Result<void>>
    update_checksum() {
        return this->derived().impl_update_checksum();
    }

protected:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
    Derived& derived() { return static_cast<Derived&>(*this); }
};

} // namespace USBCANBridge

/**
 * @brief Validation interfaces for compile-time and runtime validation
 */

#pragma once
#include "common.hpp"
#include "result.hpp"

namespace USBCANBridge {

/**
 * @brief Core validation interface for all frames
 */
template<typename Derived>
class CoreValidator {
public:
    Result<bool> validate_structure() const {
        return static_cast<const Derived*>(this)->impl_validate_structure();
    }
    
    Result<bool> validate_header() const {
        return static_cast<const Derived*>(this)->impl_validate_header();
    }
};

/**
 * @brief Data frame validation
 */
template<typename Derived>
class DataValidator : public CoreValidator<Derived> {
public:
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<bool>>
    validate_can_id(uint32_t id) const {
        return static_cast<const T*>(this)->impl_validate_can_id(id);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_data_frame_v<T>, Result<bool>>
    validate_data_length(std::size_t length) const {
        return static_cast<const T*>(this)->impl_validate_data_length(length);
    }
};

/**
 * @brief Config frame validation
 */
template<typename Derived>
class ConfigValidator : public CoreValidator<Derived> {
public:
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<bool>>
    validate_baud_rate(CANBaud rate) const {
        return static_cast<const T*>(this)->impl_validate_baud_rate(rate);
    }
    
    template<typename T = Derived>
    std::enable_if_t<is_config_frame_v<T>, Result<bool>>
    validate_mode(CANMode mode) const {
        return static_cast<const T*>(this)->impl_validate_mode(mode);
    }
};

} // namespace USBCANBridge

/**
 * @brief New FixedFrame implementation with clean interfaces
 */

#pragma once
#include "frame_interfaces.hpp"
#include "checksum_mixin.hpp"
#include "validation_interfaces.hpp"

namespace USBCANBridge {

class FixedFrame : 
    public DataFrame<FixedFrame>,
    public ChecksumMixin<FixedFrame>,
    public DataValidator<FixedFrame> {
    
private:
    using Traits = frame_traits_t<FixedFrame>;
    using Layout = layout_t<FixedFrame>;
    alignas(8) mutable storage_t<FixedFrame> storage_;
    mutable bool checksum_dirty_ = true;

public:
    FixedFrame();
    
    // FrameCore implementation
    Result<void> impl_serialize(std::span<std::byte> buffer) const;
    Result<void> impl_deserialize(std::span<const std::byte> data);
    Result<bool> impl_validate() const;
    Result<std::size_t> impl_size() const;
    Result<void> impl_clear();
    std::span<const std::byte> impl_get_raw_data() const;
    
    // DataFrame implementation
    Result<void> impl_set_can_id(uint32_t id);
    Result<uint32_t> impl_get_can_id() const;
    Result<void> impl_set_data(std::span<const std::byte> data);
    Result<std::span<const std::byte>> impl_get_data() const;
    Result<uint8_t> impl_get_dlc() const;
    Result<bool> impl_is_remote() const;
    Result<void> impl_set_frame_type(FrameType type);
    Result<FrameType> impl_get_frame_type() const;
    
    // ChecksumMixin implementation
    Result<uint8_t> impl_calculate_checksum() const;
    Result<bool> impl_verify_checksum() const;
    Result<void> impl_update_checksum();
    
    // DataValidator implementation
    Result<bool> impl_validate_structure() const;
    Result<bool> impl_validate_header() const;
    Result<bool> impl_validate_can_id(uint32_t id) const;
    Result<bool> impl_validate_data_length(std::size_t length) const;

private:
    void impl_init_fixed_fields();
};

} // namespace USBCANBridge

/**
 * @brief New ConfigFrame implementation with clean interfaces
 */

#pragma once
#include "frame_interfaces.hpp"
#include "checksum_mixin.hpp"
#include "validation_interfaces.hpp"

namespace USBCANBridge {

class ConfigFrame : 
    public ConfigInterface<ConfigFrame>,
    public ChecksumMixin<ConfigFrame>,
    public ConfigValidator<ConfigFrame> {
    
private:
    using Traits = frame_traits_t<ConfigFrame>;
    using Layout = layout_t<ConfigFrame>;
    alignas(8) mutable storage_t<ConfigFrame> storage_;
    mutable bool checksum_dirty_ = true;

public:
    ConfigFrame();
    
    // FrameCore implementation
    Result<void> impl_serialize(std::span<std::byte> buffer) const;
    Result<void> impl_deserialize(std::span<const std::byte> data);
    Result<bool> impl_validate() const;
    Result<std::size_t> impl_size() const;
    Result<void> impl_clear();
    std::span<const std::byte> impl_get_raw_data() const;
    
    // ConfigInterface implementation  
    Result<void> impl_set_baud_rate(CANBaud rate);
    Result<CANBaud> impl_get_baud_rate() const;
    Result<void> impl_set_mode(CANMode mode);
    Result<CANMode> impl_get_mode() const;
    Result<void> impl_set_filter(uint32_t filter);
    Result<uint32_t> impl_get_filter() const;
    Result<void> impl_set_mask(uint32_t mask);
    Result<uint32_t> impl_get_mask() const;
    Result<void> impl_set_auto_rtx(RTX auto_rtx);
    Result<RTX> impl_get_auto_rtx() const;
    
    // ChecksumMixin implementation
    Result<uint8_t> impl_calculate_checksum() const;
    Result<bool> impl_verify_checksum() const;
    Result<void> impl_update_checksum();
    
    // ConfigValidator implementation
    Result<bool> impl_validate_structure() const;
    Result<bool> impl_validate_header() const;
    Result<bool> impl_validate_baud_rate(CANBaud rate) const;
    Result<bool> impl_validate_mode(CANMode mode) const;

private:
    void impl_init_fixed_fields();
};

} // namespace USBCANBridge

/**
 * @brief Enhanced fluent FrameBuilder with validation and single entry point
 */

#pragma once
#include "frame_interfaces.hpp"
#include "fixed_frame_v2.hpp"
#include "variable_frame_v2.hpp"
#include "config_frame_v2.hpp"

namespace USBCANBridge {

/**
 * @brief Enhanced FrameBuilder with validation and type safety
 */
template<typename FrameType>
class FrameBuilder {
private:
    FrameType frame_;
    bool built_ = false;

public:
    FrameBuilder() = default;
    
    // === DATA FRAME OPERATIONS ===
    template<typename T = FrameType>
    std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
    can_id(uint32_t id) {
        auto validation = frame_.validate_can_id(id);
        if (!validation || !validation.value()) {
            throw std::invalid_argument("FrameBuilder: Invalid CAN ID - " + validation.describe());
        }
        
        auto result = frame_.set_can_id(id);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    template<typename T = FrameType>
    std::enable_if_t<is_data_frame_v<T>, FrameBuilder&>
    data(std::span<const std::byte> data) {
        auto validation = frame_.validate_data_length(data.size());
        if (!validation || !validation.value()) {
            throw std::invalid_argument("FrameBuilder: Invalid data length - " + validation.describe());
        }
        
        auto result = frame_.set_data(data);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    template<typename T = FrameType>
    std::enable_if_t<is_fixed_frame_v<T>, FrameBuilder&>
    frame_type(FrameType type) {
        auto result = frame_.set_frame_type(type);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    // === CONFIG FRAME OPERATIONS ===
    template<typename T = FrameType>
    std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
    baud_rate(CANBaud rate) {
        auto validation = frame_.validate_baud_rate(rate);
        if (!validation || !validation.value()) {
            throw std::invalid_argument("FrameBuilder: Invalid baud rate - " + validation.describe());
        }
        
        auto result = frame_.set_baud_rate(rate);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    template<typename T = FrameType>
    std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
    mode(CANMode mode) {
        auto validation = frame_.validate_mode(mode);
        if (!validation || !validation.value()) {
            throw std::invalid_argument("FrameBuilder: Invalid mode - " + validation.describe());
        }
        
        auto result = frame_.set_mode(mode);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    template<typename T = FrameType>
    std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
    filter(uint32_t filter) {
        auto result = frame_.set_filter(filter);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    template<typename T = FrameType>
    std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
    mask(uint32_t mask) {
        auto result = frame_.set_mask(mask);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    template<typename T = FrameType>
    std::enable_if_t<is_config_frame_v<T>, FrameBuilder&>
    auto_rtx(RTX rtx) {
        auto result = frame_.set_auto_rtx(rtx);
        if (!result) {
            throw std::runtime_error("FrameBuilder: " + result.describe());
        }
        return *this;
    }
    
    // === BUILD OPERATIONS ===
    FrameType build() {
        auto validation = frame_.validate();
        if (!validation || !validation.value()) {
            throw std::runtime_error("FrameBuilder: Frame validation failed - " + validation.describe());
        }
        
        built_ = true;
        return std::move(frame_);
    }
    
    FrameType& get() {
        if (!built_) {
            auto validation = frame_.validate();
            if (!validation || !validation.value()) {
                throw std::runtime_error("FrameBuilder: Frame validation failed - " + validation.describe());
            }
        }
        return frame_;
    }
};

// === SINGLE ENTRY POINT FACTORY ===

/**
 * @brief Single entry point for frame creation
 */
class Frame {
public:
    template<typename FrameType>
    static FrameBuilder<FrameType> create() {
        static_assert(is_data_frame_v<FrameType> || is_config_frame_v<FrameType>,
                     "Frame type must be FixedFrame, VariableFrame, or ConfigFrame");
        return FrameBuilder<FrameType>{};
    }
    
    // Convenience methods
    static FrameBuilder<FixedFrame> fixed() {
        return FrameBuilder<FixedFrame>{};
    }
    
    static FrameBuilder<VariableFrame> variable() {
        return FrameBuilder<VariableFrame>{};
    }
    
    static FrameBuilder<ConfigFrame> config() {
        return FrameBuilder<ConfigFrame>{};
    }
};

} // namespace USBCANBridge

// Example usage with the new architecture
#include "frame_builder_v2.hpp"

using namespace USBCANBridge;

// Single entry point usage as requested
auto config_frame = Frame::create<ConfigFrame>()
    .baud_rate(CANBaud::SPEED_500K)
    .mode(CANMode::NORMAL)
    .filter(0x123)
    .mask(0xFFF)
    .auto_rtx(RTX::AUTO)
    .build();

// Or using convenience methods
auto data_frame = Frame::fixed()
    .can_id(0x123)
    .data(data_bytes)
    .frame_type(FrameType::STD_FIXED)
    .build();

auto var_frame = Frame::variable()
    .can_id(0x456)
    .data(small_data)
    .build();
    