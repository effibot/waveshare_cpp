/**
 * @file data.hpp
 * @brief Data frame interface for state-first architecture
 * @version 3.0
 * @date 2025-10-09
 *
 * State-First Architecture:
 * - DataState holds format, can_id, dlc, and data vector
 * - Setters modify state only (no buffer writes)
 * - Serialization happens on-demand
 *
 * @copyright Copyright (c) 2025
 */

#pragma once
#include "core.hpp"
#include <type_traits>
#include <vector>
#include <boost/core/span.hpp>

using namespace boost;

namespace USBCANBridge {

    /**
     * @brief Data state for data frames (FixedFrame, VariableFrame)
     * Holds runtime values specific to data frames
     */
    struct DataState {
        Format format = Format::DATA_VARIABLE;      // DATA or REMOTE frame
        std::uint32_t can_id = 0;                   // CAN ID (11-bit or 29-bit)
        std::size_t dlc = 0;                        // Data Length Code (0-8)
        std::vector<std::uint8_t> data;             // Data payload (0-8 bytes)

        DataState() {
            data.reserve(8);  // Pre-allocate for efficiency
        }
    };

    /**
     * @brief Data interface for FixedFrame and VariableFrame (state-first design)
     *
     * This class provides:
     * - DataState holding format, can_id, dlc, and data
     * - Getters/setters that modify state only
     * - Utility methods for extended ID detection
     *
     * @tparam Frame The frame type (FixedFrame or VariableFrame)
     */
    template<typename Frame>
    class DataInterface : public CoreInterface<Frame> {
        // Ensure that Frame is VariableFrame or FixedFrame
        static_assert(is_data_frame_v<Frame>,
            "Frame must be a data frame type (FixedFrame or VariableFrame)");

        protected:
            // * Data state (single source of truth)
            DataState data_state_;

            // * Prevent this class from being instantiated directly
            DataInterface() : CoreInterface<Frame>() {
                static_assert(!std::is_same_v<Frame, DataInterface>,
                    "DataInterface cannot be instantiated directly");
            }

        public:

            // === Utility Methods ===

            /**
             * @brief Check if the frame is a remote frame.
             * Remote frames are determined by the FORMAT field:
             * - For Fixed frames: REMOTE_FIXED (0x02)
             * - For Variable frames: REMOTE_VARIABLE (0x01)
             * @return bool True if remote frame, false otherwise
             */
            bool is_remote() const {
                auto fmt = data_state_.format;
                auto vers = this->core_state_.can_version;

                // Check if this is a fixed or variable frame
                bool is_fixed = (vers == CANVersion::STD_FIXED || vers == CANVersion::EXT_FIXED);
                bool is_variable = (vers == CANVersion::STD_VARIABLE ||
                    vers == CANVersion::EXT_VARIABLE);

                if (is_fixed) {
                    return fmt == Format::REMOTE_FIXED;  // 0x02
                } else if (is_variable) {
                    return fmt == Format::REMOTE_VARIABLE;  // 0x01
                }

                return false;  // Unknown frame type
            }

            /**
             * @brief Check if the frame is extended (29-bit ID) or standard (11-bit ID).
             * @return bool True if extended, false if standard
             */
            bool is_extended() const {
                return this->derived().impl_is_extended();
            }

            // === Data Frame Specific Methods ===

            /**
             * @brief Get the Frame Format (DATA or REMOTE)
             * @return Format The frame format from state
             */
            Format get_format() const {
                return data_state_.format;
            }

            /**
             * @brief Set the Frame Format (DATA or REMOTE)
             * @param format The format to set
             */
            void set_format(Format format) {
                data_state_.format = format;
            }

            /**
             * @brief Get the CAN ID
             * @return std::uint32_t The CAN ID from state
             */
            std::uint32_t get_can_id() const {
                return data_state_.can_id;
            }

            /**
             * @brief Set the CAN ID
             * @param id The CAN ID to set
             * @throws std::out_of_range if ID is invalid for the current CAN version
             */
            void set_id(std::uint32_t id) {
                // Validate ID based on CAN version
                bool is_extended = this->derived().impl_is_extended();
                if (is_extended) {
                    if (id > 0x1FFFFFFF) {
                        throw std::out_of_range("Extended CAN ID must be <= 0x1FFFFFFF");
                    }
                } else {
                    if (id > 0x7FF) {
                        throw std::out_of_range("Standard CAN ID must be <= 0x7FF");
                    }
                }
                // Set ID in state
                data_state_.can_id = id;
            }

            /**
             * @brief Get the Data Length Code (DLC)
             * @return std::size_t The DLC value from state
             */
            std::size_t get_dlc() const {
                return data_state_.dlc;
            }

            /**
             * @brief Get a mutable view of the data payload
             * @return span<std::uint8_t> Mutable view of the state's data
             */
            span<std::uint8_t> get_data() {
                return span<std::uint8_t>(data_state_.data.data(), data_state_.dlc);
            }

            /**
             * @brief Get a read-only view of the data payload
             * @return span<const std::uint8_t> Const view of the state's data
             */
            span<const std::uint8_t> get_data() const {
                return span<const std::uint8_t>(data_state_.data.data(), data_state_.dlc);
            }

            /**
             * @brief Set the data payload and update DLC accordingly
             * @param data A span representing the data payload to set
             */
            void set_data(span<const std::uint8_t> data) {
                // Resize data vector and copy
                data_state_.data.resize(data.size());
                std::copy(data.begin(), data.end(), data_state_.data.begin());
                // Update DLC to match actual data size
                data_state_.dlc = data.size();
            }

            /**
             * @brief Clear data-specific state
             * Called by CoreInterface::clear()
             */
            void impl_clear() {
                data_state_ = DataState{};
            }
    };

}