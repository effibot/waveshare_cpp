#include "../include/variable_frame.hpp"
#include <algorithm>

namespace USBCANBridge {

    VariableFrame::VariableFrame() {
        init_fixed_fields();
    }

    void VariableFrame::init_fixed_fields() {
        // Start with minimum frame size (START + TYPE + 2-byte ID + END = 5 bytes)
        storage_.resize(Traits::MIN_FRAME_SIZE);
        std::fill(storage_.begin(), storage_.end(), std::byte{0});

        // Set up basic frame structure
        storage_[Layout::START_OFFSET] = to_byte(Constants::START_BYTE);
        storage_[Layout::TYPE_OFFSET] = std::byte{0}; // Will be set based on ID and data

        // Initialize with standard ID and no data
        is_extended_id_ = false;
        data_length_ = 0;

        update_frame_structure();
    }

    void VariableFrame::update_frame_structure() {
        std::size_t new_size = calculate_frame_size(is_extended_id_, data_length_);
        storage_.resize(new_size);

        // Set END byte at the correct position
        storage_[new_size - 1] = to_byte(Constants::END_BYTE);

        // Update type byte with current frame configuration
        std::byte type_byte = static_cast<std::byte>(
            (is_extended_id_ ? 1 : 0) |  // Bit 0: Extended ID flag
            (static_cast<uint8_t>(data_length_) << 1)  // Bits 1-4: DLC
        );
        storage_[Layout::TYPE_OFFSET] = type_byte;
    }

    // ==== BASEFRAME INTERFACE IMPLEMENTATIONS ====

    Result<void> VariableFrame::impl_serialize(span<std::byte> buffer) const {
        if (buffer.size() < storage_.size()) {
            return Result<void>::error(Status::WBAD_LENGTH, "serialize");
        }

        std::copy(storage_.begin(), storage_.end(), buffer.begin());
        return Result<void>::success("serialize");
    }

    Result<void> VariableFrame::impl_deserialize(span<const std::byte> data) {
        if (data.size() < Traits::MIN_FRAME_SIZE || data.size() > Traits::MAX_FRAME_SIZE) {
            return Result<void>::error(Status::WBAD_LENGTH, "deserialize");
        }

        storage_.assign(data.begin(), data.end());

        // Validate basic structure
        if (storage_[Layout::START_OFFSET] != to_byte(Constants::START_BYTE)) {
            return Result<void>::error(Status::WBAD_START, "deserialize");
        }

        if (storage_.back() != to_byte(Constants::END_BYTE)) {
            return Result<void>::error(Status::WBAD_DATA, "deserialize");
        }

        // Parse type byte to update cached values
        std::byte type_byte = storage_[Layout::TYPE_OFFSET];
        is_extended_id_ = (static_cast<uint8_t>(type_byte) & 0x01) != 0;
        data_length_ = (static_cast<uint8_t>(type_byte) >> 1) & 0x0F;

        return Result<void>::success("deserialize");
    }

    Result<bool> VariableFrame::impl_validate() const {
        if (storage_.size() < Traits::MIN_FRAME_SIZE || storage_.size() > Traits::MAX_FRAME_SIZE) {
            return Result<bool>::success(false, "validate");
        }

        if (storage_[Layout::START_OFFSET] != to_byte(Constants::START_BYTE)) {
            return Result<bool>::success(false, "validate");
        }

        if (storage_.back() != to_byte(Constants::END_BYTE)) {
            return Result<bool>::success(false, "validate");
        }

        return Result<bool>::success(true, "validate");
    }

    span<const std::byte> VariableFrame::impl_get_raw_data() const {
        return span<const std::byte>(storage_.data(), storage_.size());
    }

    Result<std::size_t> VariableFrame::impl_size() const {
        return Result<std::size_t>::success(storage_.size(), "size");
    }

    Result<void> VariableFrame::impl_clear() {
        init_fixed_fields();
        return Result<void>::success("clear");
    }

    // ==== DATA FRAME OPERATIONS ====

    Result<void> VariableFrame::impl_set_can_id(uint32_t id) {
        bool new_extended = (id > 0x7FF);

        if (new_extended && id > 0x1FFFFFFF) {
            return Result<void>::error(Status::WBAD_DATA, "set_can_id");
        }

        is_extended_id_ = new_extended;
        update_frame_structure();

        std::size_t id_offset = Layout::ID_OFFSET;

        if (is_extended_id_) {
            // Store 4-byte extended ID in big endian
            storage_[id_offset] = static_cast<std::byte>((id >> 24) & 0xFF);
            storage_[id_offset + 1] = static_cast<std::byte>((id >> 16) & 0xFF);
            storage_[id_offset + 2] = static_cast<std::byte>((id >> 8) & 0xFF);
            storage_[id_offset + 3] = static_cast<std::byte>(id & 0xFF);
        } else {
            // Store 2-byte standard ID in big endian
            storage_[id_offset] = static_cast<std::byte>((id >> 8) & 0xFF);
            storage_[id_offset + 1] = static_cast<std::byte>(id & 0xFF);
        }

        return Result<void>::success("set_can_id");
    }

    Result<uint32_t> VariableFrame::impl_get_can_id() const {
        std::size_t id_offset = Layout::ID_OFFSET;
        uint32_t id = 0;

        if (is_extended_id_) {
            id = (static_cast<uint32_t>(storage_[id_offset]) << 24) |
                (static_cast<uint32_t>(storage_[id_offset + 1]) << 16) |
                (static_cast<uint32_t>(storage_[id_offset + 2]) << 8) |
                static_cast<uint32_t>(storage_[id_offset + 3]);
        } else {
            id = (static_cast<uint32_t>(storage_[id_offset]) << 8) |
                static_cast<uint32_t>(storage_[id_offset + 1]);
        }

        return Result<uint32_t>::success(id, "get_can_id");
    }

    Result<void> VariableFrame::impl_set_data(span<const std::byte> data) {
        if (data.size() > 8) {
            return Result<void>::error(Status::WBAD_LENGTH, "set_data");
        }

        data_length_ = data.size();
        update_frame_structure();

        std::size_t data_offset = Layout::data_offset(is_extended_id_);
        std::copy(data.begin(), data.end(), storage_.begin() + data_offset);

        return Result<void>::success("set_data");
    }

    span<const std::byte> VariableFrame::impl_get_data() const {
        std::size_t data_offset = Layout::data_offset(is_extended_id_);
        return span<const std::byte>(storage_.data() + data_offset, data_length_);
    }

    Result<void> VariableFrame::impl_set_dlc(std::byte dlc) {
        uint8_t dlc_value = static_cast<uint8_t>(dlc);
        if (dlc_value > 8) {
            return Result<void>::error(Status::WBAD_DATA, "set_dlc");
        }

        data_length_ = dlc_value;
        update_frame_structure();

        return Result<void>::success("set_dlc");
    }

    Result<std::byte> VariableFrame::impl_get_dlc() const {
        return Result<std::byte>::success(static_cast<std::byte>(data_length_), "get_dlc");
    }

    // ==== VARIABLE FRAME SPECIFIC OPERATIONS ====

    bool VariableFrame::is_extended_id() const {
        return is_extended_id_;
    }

    std::size_t VariableFrame::get_data_length() const {
        return data_length_;
    }

    std::byte VariableFrame::get_type_byte() const {
        return storage_[Layout::TYPE_OFFSET];
    }

    Result<void> VariableFrame::set_type_byte(std::byte type_byte) {
        storage_[Layout::TYPE_OFFSET] = type_byte;

        // Update cached values from type byte
        is_extended_id_ = (static_cast<uint8_t>(type_byte) & 0x01) != 0;
        data_length_ = (static_cast<uint8_t>(type_byte) >> 1) & 0x0F;

        update_frame_structure();
        return Result<void>::success("set_type_byte");
    }

    // ==== UTILITIES ====

    std::size_t VariableFrame::calculate_frame_size(bool extended_id, std::size_t data_length) {
        return Layout::frame_size(extended_id, data_length);
    }

    bool VariableFrame::validate_structure() const {
        auto result = impl_validate();
        return result && result.value();
    }

} // namespace USBCANBridge