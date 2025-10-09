/**
 * @file variable_frame.hpp
 * @brief VariableFrame implementation for state-first architecture
 * @version 3.0
 * @date 2025-10-09
 *
 * State-First Architecture:
 * - State stored in CoreState and DataState
 * - Serialization on-demand via impl_serialize()
 * - No persistent buffer storage
 * - TYPE byte computed during serialization using VarTypeHelper
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include "../interface/data.hpp"
#include "../interface/serialization_helpers.hpp"

namespace USBCANBridge {
    /**
     * @brief Variable Frame implementation (5-15 bytes)
     *
     * Frame structure:
     * ```
     * [START][TYPE][ID(2/4)][DATA(0-8)][END]
     *   0      1     2-5/2-3  var       last
     * ```
     *
     * Features:
     * - Variable size (5-15 bytes) based on ID type and DLC
     * - 2-byte CAN ID for standard (11-bit)
     * - 4-byte CAN ID for extended (29-bit)
     * - 0-8 byte data payload
     * - TYPE byte encodes CAN version, format, and DLC
     * - No checksum, uses END byte (0x55)
     */
    class VariableFrame : public DataInterface<VariableFrame> {
        private:
            // Layout type alias
            using Layout = FrameTraits<VariableFrame>::Layout;

        public:
            // === Constructors ===

            /**
             * @brief Default constructor
             * Creates a VariableFrame with default values
             */
            VariableFrame() : DataInterface<VariableFrame>() {
                // Set default core state
                core_state_.can_version = CANVersion::STD_VARIABLE;
                core_state_.type = Type::DATA_VARIABLE;

                // Set default data state
                data_state_.format = Format::DATA_VARIABLE;
                data_state_.can_id = 0;
                data_state_.dlc = 0;
                data_state_.data.clear();
            }

            /**
             * @brief Constructor with parameters
             * @param fmt Frame format (DATA_VARIABLE or REMOTE_VARIABLE)
             * @param ver CAN version (STD_VARIABLE or EXT_VARIABLE)
             * @param id CAN ID (11-bit or 29-bit)
             * @param data_span Data payload (0-8 bytes)
             */
            VariableFrame(Format fmt, CANVersion ver, std::uint32_t id,
                span<const std::uint8_t> data_span = {})
                : DataInterface<VariableFrame>() {

                // Use setters to populate state
                set_format(fmt);
                set_CAN_version(ver);
                core_state_.type = Type::DATA_VARIABLE;
                set_id(id);

                if (!data_span.empty()) {
                    set_data(data_span);
                }
            }

            // === Serialization Implementation ===

            /**
             * @brief Serialize frame state to byte buffer
             * @return std::vector<std::uint8_t> Variable-size buffer (5-15 bytes)
             */
            std::vector<std::uint8_t> impl_serialize() const {
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

            /**
             * @brief Deserialize byte buffer into frame state
             * @param buffer Input buffer to parse
             * @return Result<void> Success or error status
             */
            Result<void> impl_deserialize(span<const std::uint8_t> buffer) {
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

            /**
             * @brief Get serialized size based on current state
             * @return std::size_t Size in bytes (5-15)
             */
            std::size_t impl_serialized_size() const {
                bool is_extended = (core_state_.can_version == CANVersion::EXT_VARIABLE);
                std::size_t id_size = is_extended ? 4 : 2;
                return 1 + 1 + id_size + data_state_.dlc + 1; // START + TYPE + ID + DATA + END
            }

            // === State Access Implementations ===

            /**
             * @brief Check if using extended CAN ID
             * @return bool True if extended (29-bit), false if standard (11-bit)
             */
            bool impl_is_extended() const {
                return (core_state_.can_version == CANVersion::EXT_VARIABLE);
            }

            /**
             * @brief Clear implementation - resets to defaults
             */
            void impl_clear() {
                core_state_.can_version = CANVersion::STD_VARIABLE;
                core_state_.type = Type::DATA_VARIABLE;
                data_state_ = DataState{};
            }
    };
}