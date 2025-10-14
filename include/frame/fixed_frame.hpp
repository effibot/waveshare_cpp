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

namespace waveshare {
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
            // Trait specialization ensures correct Layout
            using Traits = traits_t<FixedFrame>;
            // Layout type alias
            using Layout = Traits::Layout;

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
            std::vector<std::uint8_t> impl_serialize() const;

            /**
             * @brief Deserialize byte buffer into frame state
             * @param buffer Input buffer to parse
             * @return Result<void> Success or error status
             */
            Result<void> impl_deserialize(span<const std::uint8_t> buffer);

            /**
             * @brief Get serialized size
             * @return std::size_t Always returns 20
             */
            std::size_t impl_serialized_size() const;

            // === State Access Implementations ===

            /**
             * @brief Check if using extended CAN ID
             * @return bool True if extended (29-bit), false if standard (11-bit)
             */
            bool impl_is_extended() const;

            /**
             * @brief Clear implementation - resets to defaults
             */
            void impl_clear();
    };
} // namespace waveshare
