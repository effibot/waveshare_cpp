/**
 * @file fixed_frame.hpp
 * @brief FixedFrame implementation for state-first architecture
 * @version 3.0
 * @date 2025-10-09
 *
 * State-First Architecture:
 * - State stored in CoreState and DataState
 * - Serialization on-demand via impl_serialize()
 * - No persistent buffer storage
 * - Checksum computed during serialization
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include "../interface/data.hpp"
#include "../interface/serialization_helpers.hpp"

namespace USBCANBridge {
    /**
     * @brief Fixed Frame implementation (20 bytes)
     *
     * Frame structure:
     * ```
     * [START][HEADER][TYPE][CAN_VERS][FORMAT][ID(4)][DLC][DATA(8)][RESERVED][CHECKSUM]
     *   0      1       2      3        4      5-8     9    10-17     18        19
     * ```
     *
     * Features:
     * - Fixed 20-byte size
     * - 4-byte CAN ID (little-endian)
     * - 8-byte data field (padded with zeros if DLC < 8)
     * - Automatic checksum calculation during serialization
     */
    class FixedFrame : public DataInterface<FixedFrame> {
        private:
            // Layout type alias
            using Layout = FrameTraits<FixedFrame>::Layout;

        public:
            // === Constructors ===

            /**
             * @brief Default constructor
             * Creates a FixedFrame with default values
             */
            FixedFrame() : DataInterface<FixedFrame>() {
                // Set default core state
                core_state_.can_version = CANVersion::STD_FIXED;
                core_state_.type = Type::DATA_FIXED;

                // Set default data state
                data_state_.format = Format::DATA_FIXED;
                data_state_.can_id = 0;
                data_state_.dlc = 0;
                data_state_.data.clear();
            }

            /**
             * @brief Constructor with parameters
             * @param fmt Frame format (DATA_FIXED or REMOTE_FIXED)
             * @param ver CAN version (STD_FIXED or EXT_FIXED)
             * @param id CAN ID (11-bit or 29-bit)
             * @param data_span Data payload (0-8 bytes)
             */
            FixedFrame(Format fmt, CANVersion ver, std::uint32_t id,
                span<const std::uint8_t> data_span = {})
                : DataInterface<FixedFrame>() {

                // Use setters to populate state
                set_format(fmt);
                set_CAN_version(ver);
                core_state_.type = Type::DATA_FIXED;
                set_id(id);

                if (!data_span.empty()) {
                    set_data(data_span);
                }
            }

            // === Serialization Implementation ===

            /**
             * @brief Serialize frame state to byte buffer
             * @return std::vector<std::uint8_t> 20-byte buffer
             */
            std::vector<std::uint8_t> impl_serialize() const {
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

            /**
             * @brief Deserialize byte buffer into frame state
             * @param buffer Input buffer to parse
             * @return Result<void> Success or error status
             */
            Result<void> impl_deserialize(span<const std::uint8_t> buffer) {
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

            /**
             * @brief Get serialized size
             * @return std::size_t Always returns 20
             */
            std::size_t impl_serialized_size() const {
                return 20;
            }

            // === State Access Implementations ===

            /**
             * @brief Check if using extended CAN ID
             * @return bool True if extended (29-bit), false if standard (11-bit)
             */
            bool impl_is_extended() const {
                return (core_state_.can_version == CANVersion::EXT_FIXED);
            }

            /**
             * @brief Clear implementation - resets to defaults
             */
            void impl_clear() {
                core_state_.can_version = CANVersion::STD_FIXED;
                core_state_.type = Type::DATA_FIXED;
                data_state_ = DataState{};
            }
    };
} // namespace USBCANBridge
