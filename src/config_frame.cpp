/**
 * @file config_frame.cpp
 * @brief ConfigFrame implementation for state-first architecture
 * @version 3.0
 * @date 2025-10-09
 * State-First Architecture:
 * - State stored in CoreState and ConfigState
 * - Serialization on-demand via impl_serialize()
 * - No persistent buffer storage
 * - Checksum computed during serialization using ChecksumHelper
 */

#include "../include/frame/config_frame.hpp"

namespace waveshare {

    // === Serialization Implementation ===

    std::vector<std::uint8_t> ConfigFrame::impl_serialize() const {
        std::vector<std::uint8_t> buffer(20, 0x00);

        // Fixed protocol bytes
        buffer[Layout::START] = to_byte(Constants::START_BYTE);
        buffer[Layout::HEADER] = to_byte(Constants::HEADER);
        buffer[Layout::TYPE] = to_byte(core_state_.type);

        // Configuration bytes
        buffer[Layout::BAUD] = to_byte(config_state_.baud_rate);
        buffer[Layout::CAN_VERS] = to_byte(core_state_.can_version);

        // Filter (big-endian, 4 bytes)
        auto filter_bytes = int_to_bytes_be<std::uint32_t, 4>(config_state_.filter);
        std::copy(filter_bytes.begin(), filter_bytes.end(),
            buffer.begin() + Layout::FILTER);

        // Mask (big-endian, 4 bytes)
        auto mask_bytes = int_to_bytes_be<std::uint32_t, 4>(config_state_.mask);
        std::copy(mask_bytes.begin(), mask_bytes.end(), buffer.begin() + Layout::MASK);

        buffer[Layout::MODE] = to_byte(config_state_.can_mode);
        buffer[Layout::AUTO_RTX] = to_byte(config_state_.auto_rtx);

        // Reserved bytes (4 bytes, all 0x00)
        for (size_t i = 0; i < 4; ++i) {
            buffer[Layout::RESERVED + i] = to_byte(Constants::RESERVED);
        }

        // Compute and write checksum
        ChecksumHelper::write(buffer, Layout::CHECKSUM, Layout::TYPE, Layout::RESERVED + 3);

        return buffer;
    }

    void ConfigFrame::impl_deserialize(span<const std::uint8_t> buffer) {
        if (buffer.size() != 20) {
            throw ProtocolException(Status::WBAD_LENGTH,
                "ConfigFrame requires exactly 20 bytes");
        }

        // Validate fixed protocol bytes
        if (buffer[Layout::START] != to_byte(Constants::START_BYTE)) {
            throw ProtocolException(Status::WBAD_FORMAT,
                "Invalid START byte");
        }

        if (buffer[Layout::HEADER] != to_byte(Constants::HEADER)) {
            throw ProtocolException(Status::WBAD_FORMAT,
                "Invalid HEADER byte");
        }

        // Validate checksum
        if (!ChecksumHelper::validate(buffer, Layout::CHECKSUM, Layout::TYPE,
            Layout::RESERVED + 3)) {
            throw ProtocolException(Status::WBAD_CHECKSUM,
                "Checksum validation failed");
        }

        // Extract state from buffer
        core_state_.type = from_byte<Type>(buffer[Layout::TYPE]);
        core_state_.can_version = from_byte<CANVersion>(buffer[Layout::CAN_VERS]);
        config_state_.baud_rate = from_byte<CANBaud>(buffer[Layout::BAUD]);
        config_state_.can_mode = from_byte<CANMode>(buffer[Layout::MODE]);
        config_state_.auto_rtx = from_byte<RTX>(buffer[Layout::AUTO_RTX]);

        // Extract filter (big-endian, 4 bytes)
        config_state_.filter = bytes_to_int_be<std::uint32_t>(
            buffer.subspan(Layout::FILTER, 4)
        );

        // Extract mask (big-endian, 4 bytes)
        config_state_.mask = bytes_to_int_be<std::uint32_t>(
            buffer.subspan(Layout::MASK, 4)
        );
    }


} // namespace USBCANBridge