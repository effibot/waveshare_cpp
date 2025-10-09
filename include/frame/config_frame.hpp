/**
 * @file config_frame.hpp
 * @brief ConfigFrame implementation for state-first architecture
 * @version 3.0
 * @date 2025-10-09
 *
 * State-First Architecture:
 * - State stored in CoreState and ConfigState
 * - Serialization on-demand via impl_serialize()
 * - No persistent buffer storage
 * - Checksum computed during serialization using ChecksumHelper
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

#include "../interface/config.hpp"
#include "../interface/serialization_helpers.hpp"

namespace USBCANBridge {
    /**
     * @brief Configuration frame for the USB-CAN Bridge (20 bytes)
     *
     * Frame structure:
     * ```
     * [START][HEADER][TYPE][CAN_BAUD][CAN_VERS][FILTER(4)][MASK(4)][CAN_MODE][AUTO_RTX][RESERVED(4)][CHECKSUM]
     * ```
     *
     * Features:
     * - Fixed 20-byte frame
     * - Configures baud rate, mode, filters, and masks
     * - Checksum validation over bytes 2-14
     */
    class ConfigFrame : public ConfigInterface<ConfigFrame> {
        private:
            // Layout type alias
            using Layout = FrameTraits<ConfigFrame>::Layout;

        public:
            // === Constructors ===

            /**
             * @brief Default constructor
             * Creates a ConfigFrame with default configuration values
             */
            ConfigFrame() : ConfigInterface<ConfigFrame>() {
                // Set default core state
                core_state_.type = Type::CONF_FIXED;
                core_state_.can_version = CANVersion::STD_FIXED;

                // Set default config state
                config_state_.baud_rate = CANBaud::BAUD_1M;
                config_state_.can_mode = CANMode::NORMAL;
                config_state_.auto_rtx = RTX::AUTO;
                config_state_.filter = 0;
                config_state_.mask = 0;
            }

            /**
             * @brief Constructor with parameters
             * @param type Frame type (CONF_FIXED or CONF_VARIABLE)
             * @param baud Baud rate setting
             * @param mode CAN mode (NORMAL, LOOPBACK, LISTENONLY)
             * @param auto_rtx Automatic retransmission setting
             * @param filter Acceptance filter value (big-endian)
             * @param mask Acceptance mask value (big-endian)
             * @param can_vers CAN version
             */
            ConfigFrame(
                Type type,
                CANBaud baud,
                CANMode mode,
                RTX auto_rtx,
                std::uint32_t filter,
                std::uint32_t mask,
                CANVersion can_vers
            ) : ConfigInterface<ConfigFrame>() {
                core_state_.type = type;
                core_state_.can_version = can_vers;
                config_state_.baud_rate = baud;
                config_state_.can_mode = mode;
                config_state_.auto_rtx = auto_rtx;
                config_state_.filter = filter;
                config_state_.mask = mask;
            }

            // === Serialization Implementation ===

            /**
             * @brief Serialize frame state to 20-byte buffer
             * @return std::vector<std::uint8_t> 20-byte buffer with checksum
             */
            std::vector<std::uint8_t> impl_serialize() const {
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

            /**
             * @brief Deserialize byte buffer into frame state
             * @param buffer Input buffer to parse (must be 20 bytes)
             * @return Result<void> Success or error status
             */
            Result<void> impl_deserialize(span<const std::uint8_t> buffer) {
                if (buffer.size() != 20) {
                    return Result<void>::error(Status::WBAD_LENGTH,
                        "ConfigFrame requires exactly 20 bytes");
                }

                // Validate fixed protocol bytes
                if (buffer[Layout::START] != to_byte(Constants::START_BYTE)) {
                    return Result<void>::error(Status::WBAD_FORMAT,
                        "Invalid START byte");
                }

                if (buffer[Layout::HEADER] != to_byte(Constants::HEADER)) {
                    return Result<void>::error(Status::WBAD_FORMAT,
                        "Invalid HEADER byte");
                }

                // Validate checksum
                if (!ChecksumHelper::validate(buffer, Layout::CHECKSUM, Layout::TYPE,
                    Layout::RESERVED + 3)) {
                    return Result<void>::error(Status::WBAD_CHECKSUM,
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

                return Result<void>::success();
            }

            /**
             * @brief Get serialized size (always 20 bytes)
             * @return std::size_t Size in bytes
             */
            std::size_t impl_serialized_size() const {
                return 20;
            }

            // === State Access Implementations ===

            /**
             * @brief Clear implementation - resets to defaults
             */
            void impl_clear() {
                core_state_.type = Type::CONF_FIXED;
                core_state_.can_version = CANVersion::STD_FIXED;
                config_state_ = ConfigState{};
            }
    };
}