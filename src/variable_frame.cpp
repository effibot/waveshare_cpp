/**
 * @file variable_frame.cpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Variable Frame implementation for state-first architecture
 * @version 0.1
 * @date 2025-10-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "../include/frame/variable_frame.hpp"

namespace USBCANBridge {

    // === Serialization Implementation ===

    std::vector<std::uint8_t> VariableFrame::impl_serialize() const {
        bool is_extended = (core_state_.can_version == CANVersion::EXT_VARIABLE);
        std::size_t id_size = is_extended ? 4 : 2;
        std::size_t frame_size = 1 + 1 + id_size + data_state_.dlc + 1; // START + TYPE + ID + DATA + END

        std::vector<std::uint8_t> buffer(frame_size, 0x00);

        // Fixed protocol bytes
        buffer[Layout::START] = to_byte(Constants::START_BYTE);

        // Compute TYPE byte using VarTypeHelper
        buffer[Layout::TYPE] = VarTypeHelper::compute_type(
            core_state_.can_version,
            data_state_.format,
            data_state_.dlc
        );

        // CAN ID (little-endian, 2 or 4 bytes)
        if (is_extended) {
            auto id_bytes = int_to_bytes_le<std::uint32_t, 4>(data_state_.can_id);
            std::copy(id_bytes.begin(), id_bytes.end(), buffer.begin() + Layout::ID);
        } else {
            auto id_bytes = int_to_bytes_le<std::uint32_t, 2>(data_state_.can_id);
            std::copy(id_bytes.begin(), id_bytes.end(), buffer.begin() + Layout::ID);
        }

        // Data (0-8 bytes)
        std::size_t data_offset = Layout::ID + id_size;
        std::copy_n(data_state_.data.begin(), data_state_.dlc,
            buffer.begin() + data_offset);

        // END byte
        buffer[frame_size - 1] = to_byte(Constants::END_BYTE);

        return buffer;
    }

    Result<void> VariableFrame::impl_deserialize(span<const std::uint8_t> buffer) {
        if (buffer.size() < 5) {
            return Result<void>::error(Status::WBAD_LENGTH,
                "VariableFrame requires at least 5 bytes");
        }

        if (buffer.size() > 15) {
            return Result<void>::error(Status::WBAD_LENGTH,
                "VariableFrame cannot exceed 15 bytes");
        }

        // Validate START and END bytes
        if (buffer[0] != to_byte(Constants::START_BYTE)) {
            return Result<void>::error(Status::WBAD_FORMAT,
                "Invalid START byte");
        }

        if (buffer[buffer.size() - 1] != to_byte(Constants::END_BYTE)) {
            return Result<void>::error(Status::WBAD_FORMAT,
                "Invalid END byte");
        }

        // Parse TYPE byte
        std::uint8_t type_byte = buffer[Layout::TYPE];
        auto [can_vers, format, dlc] = VarTypeHelper::parse_type(type_byte);

        // Determine ID size
        bool is_extended = VarTypeHelper::is_extended(type_byte);
        std::size_t id_size = is_extended ? 4 : 2;
        std::size_t data_offset = Layout::ID + id_size;
        std::size_t expected_size = 1 + 1 + id_size + dlc + 1;

        // Validate frame size
        if (buffer.size() != expected_size) {
            return Result<void>::error(Status::WBAD_LENGTH,
                "Buffer size doesn't match TYPE byte specification");
        }

        // Extract state from buffer
        core_state_.can_version = can_vers;
        core_state_.type = Type::DATA_VARIABLE;
        data_state_.format = format;
        data_state_.dlc = dlc;

        // Extract CAN ID (little-endian)
        if (is_extended) {
            data_state_.can_id = bytes_to_int_le<std::uint32_t>(
                buffer.subspan(Layout::ID, 4)
            );
        } else {
            data_state_.can_id = bytes_to_int_le<std::uint32_t>(
                buffer.subspan(Layout::ID, 2)
            );
        }

        // Extract data
        data_state_.data.resize(dlc);
        if (dlc > 0) {
            std::copy_n(buffer.begin() + data_offset, dlc, data_state_.data.begin());
        }

        return Result<void>::success();
    }

    std::size_t VariableFrame::impl_serialized_size() const {
        bool is_extended = impl_is_extended();
        return Layout::frame_size(is_extended, data_state_.dlc);
    }

    // === State Access Implementations ===

    bool VariableFrame::impl_is_extended() const {
        return (core_state_.can_version == CANVersion::EXT_VARIABLE);
    }

    void VariableFrame::impl_clear() {
        core_state_.can_version = CANVersion::STD_VARIABLE;
        core_state_.type = Type::DATA_VARIABLE;
        data_state_ = DataState{};
    }

}