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
            // Trait specialization ensures correct Layout
            using Traits = traits_t<ConfigFrame>;
            // Layout type alias
            using Layout = Traits::Layout;

        public:
            // === Constructors ===

            /**
             * @brief Default constructor
             * Creates a ConfigFrame with default configuration values
             * - Type: CONF_FIXED
             * - CAN Version: STD_FIXED
             * - Baud Rate: 1Mbps
             * - CAN Mode: NORMAL
             * - Auto Retransmission: AUTO
             * - Filter: 0x00000000
             * - Mask: 0x00000000
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
            std::vector<std::uint8_t> impl_serialize() const;

            /**
             * @brief Deserialize byte buffer into frame state
             * @param buffer Input buffer to parse (must be 20 bytes)
             * @return Result<void> Success or error status
             */
            Result<void> impl_deserialize(span<const std::uint8_t> buffer);

            /**
             * @brief Get serialized size (always 20 bytes)
             * @return std::size_t Size in bytes
             */
            std::size_t impl_serialized_size() const {
                return Traits::FRAME_SIZE;
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