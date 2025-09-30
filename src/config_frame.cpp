#include "../include/config_frame.hpp"
#include <algorithm>

namespace USBCANBridge {

    ConfigFrame::ConfigFrame() {
        impl_init_fixed_fields();
    }

    void ConfigFrame::impl_init_fixed_fields() {
        std::fill(storage_.begin(), storage_.end(), std::byte{0});

        // Set up configuration frame structure
        storage_[Layout::START_OFFSET] = to_byte(Constants::START_BYTE);
        storage_[Layout::HEADER_OFFSET] = to_byte(Constants::MSG_HEADER);
        storage_[Layout::TYPE_OFFSET] = to_byte(Type::CONF_VAR); // Default type
        storage_[Layout::BAUD_OFFSET] = to_byte(CANBaud::SPEED_125K);  // Default baud rate
        storage_[Layout::FRAME_TYPE_OFFSET] = to_byte(FrameType::STD_FIXED); // Default frame type
        // * [Filter ID] defaulted with zero-filling
        // for (std::size_t i = 0; i < Layout::FILTER_SIZE; ++i) {
        //     storage_[Layout::FILTER_OFFSET + i] = std::byte{0};
        // }
        // * [Mask ID] defaulted with zero-filling
        // for (std::size_t i = 0; i < Layout::MASK_SIZE; ++i) {
        //     storage_[Layout::MASK_OFFSET + i] = std::byte{0};
        // }
        storage_[Layout::MODE_OFFSET] = to_byte(CANMode::NORMAL);    // Default mode
        // * [Auto-RTX] defaulted with zero-filling
        // storage_[Layout::AUTO_RTX_OFFSET] = to_byte(RTX::AUTO);     // Default RTX
        // * [Reserved] defaulted with zero-filling
        // for (std::size_t i = 0; i < Layout::RESERVED_SIZE; ++i) {
        //     storage_[Layout::RESERVED_OFFSET + i] = std::byte{0};
        // }
        checksum_dirty_ = true;
    }

    void ConfigFrame::update_checksum() const {
        if (checksum_dirty_) {
            storage_[Layout::CHECKSUM_OFFSET] = compute_checksum<ConfigFrame>(storage_);
        }
        checksum_dirty_ = false;
    }

    // ==== BASEFRAME INTERFACE IMPLEMENTATIONS ====

    Result<void> ConfigFrame::impl_serialize(span<std::byte> buffer) const {
        if (buffer.size() < Traits::FRAME_SIZE) {
            return Result<void>::error(Status::WBAD_LENGTH, "serialize");
        }

        update_checksum();
        std::copy(storage_.begin(), storage_.end(), buffer.begin());
        return Result<void>::success("serialize");
    }

    Result<void> ConfigFrame::impl_deserialize(span<const std::byte> data) {
        if (data.size() < Traits::FRAME_SIZE) {
            return Result<void>::error(Status::WBAD_LENGTH, "deserialize");
        }

        std::copy(data.begin(), data.begin() + Traits::FRAME_SIZE, storage_.begin());

        if (storage_[Layout::START_OFFSET] != to_byte(Constants::START_BYTE)) {
            return Result<void>::error(Status::WBAD_START, "deserialize");
        }

        if (storage_[Layout::HEADER_OFFSET] != to_byte(Constants::MSG_HEADER)) {
            return Result<void>::error(Status::WBAD_HEADER, "deserialize");
        }

        if (!impl_verify_checksum()) {
            return Result<void>::error(Status::WBAD_CHECKSUM, "deserialize");
        }

        checksum_dirty_ = false;

        return Result<void>::success("deserialize");
    }

    Result<bool> ConfigFrame::impl_validate() const {
        if (storage_[Layout::START_OFFSET] != to_byte(Constants::START_BYTE)) {
            return Result<bool>::success(false, "validate");
        }

        if (storage_[Layout::HEADER_OFFSET] != to_byte(Constants::MSG_HEADER)) {
            return Result<bool>::success(false, "validate");
        }

        bool checksum_valid = impl_verify_checksum();
        return Result<bool>::success(checksum_valid, "validate");
    }

    span<const std::byte> ConfigFrame::impl_get_raw_data() const {
        return span<const std::byte>(storage_.data(), storage_.size());
    }

    Result<std::size_t> ConfigFrame::impl_size() const {
        return Result<std::size_t>::success(Traits::FRAME_SIZE, "size");
    }

    Result<void> ConfigFrame::impl_clear() {
        impl_init_fixed_fields();
        return Result<void>::success("clear");
    }

    // ==== CONFIG FRAME OPERATIONS ====

    Result<void> ConfigFrame::impl_set_baud_rate(CANBaud rate) {
        storage_[Layout::BAUD_OFFSET] = to_byte(rate);
        checksum_dirty_ = true;
        return Result<void>::success("set_baud_rate");
    }

    Result<CANBaud> ConfigFrame::impl_get_baud_rate() const {
        return Result<CANBaud>::success(from_byte<CANBaud>(storage_[Layout::BAUD_OFFSET]),
            "get_baud_rate");
    }

    Result<void> ConfigFrame::impl_set_filter(uint32_t filter) {
        // Store filter in big endian format
        storage_[Layout::FILTER_OFFSET] = static_cast<std::byte>((filter >> 24) & 0xFF);
        storage_[Layout::FILTER_OFFSET + 1] = static_cast<std::byte>((filter >> 16) & 0xFF);
        storage_[Layout::FILTER_OFFSET + 2] = static_cast<std::byte>((filter >> 8) & 0xFF);
        storage_[Layout::FILTER_OFFSET + 3] = static_cast<std::byte>(filter & 0xFF);

        checksum_dirty_ = true;
        return Result<void>::success("set_filter");
    }

    Result<uint32_t> ConfigFrame::impl_get_filter() const {
        uint32_t filter = (static_cast<uint32_t>(storage_[Layout::FILTER_OFFSET]) << 24) |
            (static_cast<uint32_t>(storage_[Layout::FILTER_OFFSET + 1]) << 16) |
            (static_cast<uint32_t>(storage_[Layout::FILTER_OFFSET + 2]) << 8) |
            static_cast<uint32_t>(storage_[Layout::FILTER_OFFSET + 3]);

        return Result<uint32_t>::success(filter, "get_filter");
    }

    Result<void> ConfigFrame::impl_set_mask(uint32_t mask) {
        // Store mask in big endian format
        storage_[Layout::MASK_OFFSET] = static_cast<std::byte>((mask >> 24) & 0xFF);
        storage_[Layout::MASK_OFFSET + 1] = static_cast<std::byte>((mask >> 16) & 0xFF);
        storage_[Layout::MASK_OFFSET + 2] = static_cast<std::byte>((mask >> 8) & 0xFF);
        storage_[Layout::MASK_OFFSET + 3] = static_cast<std::byte>(mask & 0xFF);

        checksum_dirty_ = true;
        return Result<void>::success("set_mask");
    }

    Result<uint32_t> ConfigFrame::impl_get_mask() const {
        uint32_t mask = (static_cast<uint32_t>(storage_[Layout::MASK_OFFSET]) << 24) |
            (static_cast<uint32_t>(storage_[Layout::MASK_OFFSET + 1]) << 16) |
            (static_cast<uint32_t>(storage_[Layout::MASK_OFFSET + 2]) << 8) |
            static_cast<uint32_t>(storage_[Layout::MASK_OFFSET + 3]);

        return Result<uint32_t>::success(mask, "get_mask");
    }

    // ==== CONFIG FRAME SPECIFIC OPERATIONS ====

    Result<void> ConfigFrame::set_mode(CANMode mode) {
        storage_[Layout::MODE_OFFSET] = to_byte(mode);
        checksum_dirty_ = true;
        return Result<void>::success("set_mode");
    }

    Result<CANMode> ConfigFrame::get_mode() const {
        return Result<CANMode>::success(from_byte<CANMode>(storage_[Layout::MODE_OFFSET]),
            "get_mode");
    }

    bool ConfigFrame::impl_verify_checksum() const {
        return validate_checksum(storage_.data(), storage_.size());
    }

    // ==== CHECKSUM UTILITIES ====

    uint8_t ConfigFrame::calculate_checksum(const std::byte* data, std::size_t size) {
        std::byte checksum{0};
        for (std::size_t i = 0;
            i < std::min(size, static_cast < std::size_t > (Layout::CHECKSUM_OFFSET)); ++i) {
            checksum ^= data[i];
        }
        return static_cast<uint8_t>(checksum);
    }

    

} // namespace USBCANBridge