#include "../include/fixed_frame.hpp"
#include <algorithm>

namespace USBCANBridge {

    FixedFrame::FixedFrame() {
        impl_init_fixed_fields();
    }

    void FixedFrame::impl_init_fixed_fields() {
        storage_.fill(std::byte{0});

        // Set up standard frame structure
        storage_[Layout::START_OFFSET] = to_byte(Constants::START_BYTE);
        storage_[Layout::HEADER_OFFSET] = to_byte(Constants::MSG_HEADER);
        storage_[Layout::TYPE_OFFSET] = to_byte(Type::DATA_FIXED);
        storage_[Layout::FRAME_TYPE_OFFSET] = to_byte(FrameType::STD_FIXED);
        storage_[Layout::FORMAT_OFFSET] = to_byte(FrameFormat::DATA_FIXED);
        // * [ID (4 bytes) | DLC (1 byte) | Data (8 bytes) | Checksum (1 byte) ]
        // * Are initialized with zero-fill above
        checksum_dirty_ = true;
    }

    void FixedFrame::update_checksum() const {
        if (checksum_dirty_) {
            std::byte checksum{0};
            for (std::size_t i = 0; i < Layout::CHECKSUM_OFFSET; ++i) {
                checksum ^= storage_[i];
            }
            cached_checksum_ = checksum;
            storage_[Layout::CHECKSUM_OFFSET] = cached_checksum_;
            checksum_dirty_ = false;
        }
    }

    // ==== BASEFRAME INTERFACE IMPLEMENTATIONS ====

    Result<void> FixedFrame::impl_serialize(span<std::byte> buffer) const {
        if (buffer.size() < Traits::FRAME_SIZE) {
            return Result<void>::error(Status::WBAD_LENGTH, "serialize");
        }

        update_checksum();
        std::copy(storage_.begin(), storage_.end(), buffer.begin());

        return Result<void>::success("serialize");
    }

    Result<void> FixedFrame::impl_deserialize(span<const std::byte> data) {
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
        cached_checksum_ = storage_[Layout::CHECKSUM_OFFSET];

        return Result<void>::success("deserialize");
    }

    Result<bool> FixedFrame::impl_validate() const {
        if (storage_[Layout::START_OFFSET] != to_byte(Constants::START_BYTE)) {
            return Result<bool>::success(false, "validate");
        }

        if (storage_[Layout::HEADER_OFFSET] != to_byte(Constants::MSG_HEADER)) {
            return Result<bool>::success(false, "validate");
        }

        bool checksum_valid = impl_verify_checksum();
        return Result<bool>::success(checksum_valid, "validate");
    }

    span<const std::byte> FixedFrame::impl_get_raw_data() const {
        return span<const std::byte>(storage_.data(), storage_.size());
    }

    Result<std::size_t> FixedFrame::impl_size() const {
        return Result<std::size_t>::success(Traits::FRAME_SIZE, "size");
    }

    Result<void> FixedFrame::impl_clear() {
        impl_init_fixed_fields();
        return Result<void>::success("clear");
    }

    // ==== DATA FRAME OPERATIONS ====

    Result<void> FixedFrame::impl_set_can_id(uint32_t id) {
        // Auto-detect frame type based on ID value
        FrameType frame_type = (id > 0x7FF) ? FrameType::EXT_FIXED : FrameType::STD_FIXED;

        auto type_result = impl_set_frame_type(frame_type);
        if (!type_result) {
            return type_result;
        }

        if (frame_type == FrameType::STD_FIXED) {
            if (id > 0x7FF) {
                return Result<void>::error(Status::WBAD_DATA, "set_can_id");
            }

            storage_[Layout::ID_OFFSET] = static_cast<std::byte>((id >> 8) & 0xFF);
            storage_[Layout::ID_OFFSET + 1] = static_cast<std::byte>(id & 0xFF);

        } else {
            if (id > 0x1FFFFFFF) {
                return Result<void>::error(Status::WBAD_DATA, "set_can_id");
            }

            storage_[Layout::ID_OFFSET] = static_cast<std::byte>((id >> 24) & 0xFF);
            storage_[Layout::ID_OFFSET + 1] = static_cast<std::byte>((id >> 16) & 0xFF);
            storage_[Layout::ID_OFFSET + 2] = static_cast<std::byte>((id >> 8) & 0xFF);
            storage_[Layout::ID_OFFSET + 3] = static_cast<std::byte>(id & 0xFF);
        }

        checksum_dirty_ = true;
        return Result<void>::success("set_can_id");
    }

    Result<uint32_t> FixedFrame::impl_get_can_id() const {
        auto frame_type_result = get_frame_type();
        if (!frame_type_result) {
            return Result<uint32_t>::error(frame_type_result.error(), "get_can_id");
        }

        FrameType frame_type = frame_type_result.value();
        uint32_t id = 0;

        if (frame_type == FrameType::STD_FIXED) {
            id = (static_cast<uint32_t>(storage_[Layout::ID_OFFSET]) << 8) |
                static_cast<uint32_t>(storage_[Layout::ID_OFFSET + 1]);
        } else {
            id = (static_cast<uint32_t>(storage_[Layout::ID_OFFSET]) << 24) |
                (static_cast<uint32_t>(storage_[Layout::ID_OFFSET + 1]) << 16) |
                (static_cast<uint32_t>(storage_[Layout::ID_OFFSET + 2]) << 8) |
                static_cast<uint32_t>(storage_[Layout::ID_OFFSET + 3]);
        }

        return Result<uint32_t>::success(id, "get_can_id");
    }

    Result<void> FixedFrame::impl_set_data(span<const std::byte> data) {
        if (data.size() > Layout::DATA_SIZE) {
            return Result<void>::error(Status::WBAD_LENGTH, "set_data");
        }

        std::fill(storage_.begin() + Layout::DATA_OFFSET,
            storage_.begin() + Layout::DATA_OFFSET + Layout::DATA_SIZE,
            std::byte{0});

        std::copy(data.begin(), data.end(), storage_.begin() + Layout::DATA_OFFSET);

        auto dlc_result = impl_set_dlc(static_cast<std::byte>(data.size()));
        if (!dlc_result) {
            return dlc_result;
        }

        checksum_dirty_ = true;
        return Result<void>::success("set_data");
    }

    span<const std::byte> FixedFrame::impl_get_data() const {
        auto dlc_result = impl_get_dlc();
        std::size_t actual_size = dlc_result ? static_cast<std::size_t>(dlc_result.value()) : 0;
        actual_size = std::min(actual_size, static_cast<std::size_t>(Layout::DATA_SIZE));

        return span<const std::byte>(storage_.data() + Layout::DATA_OFFSET, actual_size);
    }

    Result<void> FixedFrame::impl_set_dlc(std::byte dlc) {
        if (static_cast<uint8_t>(dlc) > 8) {
            return Result<void>::error(Status::WBAD_DATA, "set_dlc");
        }

        storage_[Layout::DLC_OFFSET] = dlc;
        checksum_dirty_ = true;
        return Result<void>::success("set_dlc");
    }

    Result<std::byte> FixedFrame::impl_get_dlc() const {
        return Result<std::byte>::success(storage_[Layout::DLC_OFFSET], "get_dlc");
    }

    // ==== FIXED FRAME SPECIFIC OPERATIONS ====

    Result<void> FixedFrame::impl_set_frame_type(FrameType type) {
        storage_[Layout::TYPE_OFFSET] = to_byte(type);
        checksum_dirty_ = true;
        return Result<void>::success("set_frame_type");
    }

    bool FixedFrame::impl_verify_checksum() const {
        std::byte calculated{0};
        for (std::size_t i = 0; i < Layout::CHECKSUM_OFFSET; ++i) {
            calculated ^= storage_[i];
        }
        return calculated == storage_[Layout::CHECKSUM_OFFSET];
    }

    // ==== ADDITIONAL UTILITIES ====

    bool FixedFrame::is_extended_id() const {
        auto frame_type_result = get_frame_type();
        if (!frame_type_result) {
            return false;
        }
        return frame_type_result.value() == FrameType::EXT_FIXED;
    }

    Result<FrameType> FixedFrame::get_frame_type() const {
        return Result<FrameType>::success(from_byte<FrameType>(storage_[Layout::TYPE_OFFSET]),
            "get_frame_type");
    }

    // ==== CHECKSUM UTILITIES ====

    uint8_t FixedFrame::calculate_checksum(const std::byte* data, std::size_t size) {
        std::byte checksum{0};
        for (std::size_t i = 0;
            i < std::min(size, static_cast < std::size_t > (Layout::CHECKSUM_OFFSET)); ++i) {
            checksum ^= data[i];
        }
        return static_cast<uint8_t>(checksum);
    }

    bool FixedFrame::validate_checksum(const std::byte* data, std::size_t size) {
        if (size < Traits::FRAME_SIZE) {
            return false;
        }

        std::byte calculated{0};
        for (std::size_t i = 0; i < Layout::CHECKSUM_OFFSET; ++i) {
            calculated ^= data[i];
        }
        return calculated == data[Layout::CHECKSUM_OFFSET];
    }

} // namespace USBCANBridge