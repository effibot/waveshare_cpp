/**
 * @file fixed_frame.cpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief FixedFrame implementation for state-first architecture
 * @version 0.1
 * @date 2025-10-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/frame/fixed_frame.hpp"

namespace waveshare {

    // === Serialization Implementation ===

    std::vector<std::uint8_t> FixedFrame::impl_serialize()
    const {
        std::vector<std::uint8_t> buffer(20, 0x00);

        // Fixed protocol bytes
        buffer[Layout::START] = to_byte(Constants::START_BYTE);
        buffer[Layout::HEADER] = to_byte(Constants::HEADER);
        buffer[Layout::TYPE] = to_byte(Type::DATA_FIXED);
        buffer[Layout::RESERVED] = to_byte(Constants::RESERVED);

        // State-driven bytes
        buffer[Layout::CAN_VERS] = to_byte(core_state_.can_version);
        buffer[Layout::FORMAT] = to_byte(data_state_.format);
        buffer[Layout::DLC] = static_cast<std::uint8_t>(data_state_.dlc);

        // CAN ID (little-endian, 4 bytes)
        auto id_bytes = int_to_bytes_le<std::uint32_t, 4>(data_state_.can_id);
        std::copy(id_bytes.begin(), id_bytes.end(), buffer.begin() + Layout::ID);

        // Data (8 bytes, padded with zeros)
        std::size_t copy_size = std::min(data_state_.dlc, std::size_t(8));
        std::copy_n(data_state_.data.begin(), copy_size, buffer.begin() + Layout::DATA);

        // Compute and write checksum (TYPE to RESERVED inclusive)
        ChecksumHelper::write(buffer, Layout::CHECKSUM,
            Layout::CHECKSUM_START,
            Layout::CHECKSUM_END + 1);

        return buffer;
    }

    Result<void> FixedFrame::impl_deserialize(span<const std::uint8_t> buffer) {
        if (buffer.size() < 20) {
            return Result<void>::error(Status::WBAD_LENGTH,
                "FixedFrame requires exactly 20 bytes");
        }

        // Validate checksum
        if (!ChecksumHelper::validate(buffer, Layout::CHECKSUM,
            Layout::CHECKSUM_START,
            Layout::CHECKSUM_END + 1)) {
            return Result<void>::error(Status::WBAD_CHECKSUM,
                "Checksum validation failed");
        }

        // Extract state from buffer
        core_state_.can_version = from_byte<CANVersion>(buffer[Layout::CAN_VERS]);
        core_state_.type = Type::DATA_FIXED;

        data_state_.format = from_byte<Format>(buffer[Layout::FORMAT]);
        data_state_.dlc = buffer[Layout::DLC];

        // Extract CAN ID (little-endian)
        data_state_.can_id = bytes_to_int_le<std::uint32_t>(
            buffer.subspan(Layout::ID, 4)
        );

        // Extract data
        data_state_.data.resize(8);
        std::copy_n(buffer.begin() + Layout::DATA, 8, data_state_.data.begin());

        return Result<void>::success();
    }

    std::size_t FixedFrame::impl_serialized_size() const {
        return Traits::FRAME_SIZE;
    }

    // === State Access Implementations ===

    bool FixedFrame::impl_is_extended() const {
        return (core_state_.can_version == CANVersion::EXT_FIXED);
    }

    void FixedFrame::impl_clear() {
        core_state_.can_version = CANVersion::STD_FIXED;
        core_state_.type = Type::DATA_FIXED;
        data_state_ = DataState{};
    }

} // namespace USBCANBridge
