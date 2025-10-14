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

namespace waveshare {
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
     *
     * - Variable size (5-15 bytes) based on ID type and DLC
     *
     * - 2-byte CAN ID for standard (11-bit)
     *
     * - 4-byte CAN ID for extended (29-bit)
     *
     * - 0-8 byte data payload
     *
     * - TYPE byte encodes CAN version, format, and DLC
     *
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
            std::vector<std::uint8_t> impl_serialize() const;

            /**
             * @brief Deserialize byte buffer into frame state
             * @param buffer Input buffer to parse
             * @return Result<void> Success or error status
             */
            Result<void> impl_deserialize(span<const std::uint8_t> buffer);

            /**
             * @brief Get serialized size based on current state
             * @return std::size_t Size in bytes (5-15)
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
}