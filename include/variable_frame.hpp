#pragma once

#include "base_frame.hpp"
#include "span_compat.hpp"

namespace USBCANBridge {
    /**
     * @brief Variable frame implementation.
     *
     * This class represents a variable frame in the USBCANBridge library.
     * It inherits from BaseFrame and provides specific functionality for
     * variable-size frames (5-15 bytes) with dynamic CAN ID and data payload.
     *
     * Features:
     * - Variable size (5-15 bytes) with dynamic allocation
     * - Support for standard (11-bit) and extended (29-bit) CAN IDs
     * - Variable data payload (0-8 bytes)
     * - Type-safe operations through BaseFrame interface
     * - No checksum - uses END byte terminator
     *
     * Structure: [START][TYPE][ID0-ID3][DATA0-DATA7][END]
     */
    class VariableFrame : public BaseFrame<VariableFrame> {
        private:
            using Traits = frame_traits_t<VariableFrame>;
            using Layout = layout_t<VariableFrame>;

            mutable typename Traits::StorageType storage_;

            // Cache for frame properties
            mutable bool is_extended_id_ = false;
            mutable std::size_t data_length_ = 0;

            /**
             * @brief Initialize fixed fields of the frame.
             */
            void init_fixed_fields();

            /**
             * @brief Update the frame size based on current ID type and data length.
             */
            void update_frame_structure();

        public:
            /**
             * @brief Default constructor.
             * Initializes a VariableFrame with minimum size and proper field setup.
             */
            VariableFrame();

            ~VariableFrame() = default;

            // ==== BASEFRAME INTERFACE IMPLEMENTATIONS ====

            // Universal operations
            Result<void> impl_serialize(span<std::byte> buffer) const;
            Result<void> impl_deserialize(span<const std::byte> data);
            Result<bool> impl_validate() const;
            span<const std::byte> impl_get_raw_data() const;
            Result<std::size_t> impl_size() const;
            Result<void> impl_clear();

            // Data frame operations
            Result<void> impl_set_can_id(uint32_t id);
            Result<uint32_t> impl_get_can_id() const;
            Result<void> impl_set_data(span<const std::byte> data);
            span<const std::byte> impl_get_data() const;
            Result<void> impl_set_dlc(std::byte dlc);
            Result<std::byte> impl_get_dlc() const;

            // ==== VARIABLE FRAME SPECIFIC OPERATIONS ====

            /**
             * @brief Check if frame uses extended CAN ID.
             */
            bool is_extended_id() const;

            /**
             * @brief Get current data length.
             */
            std::size_t get_data_length() const;

            /**
             * @brief Get the type byte (includes frame type, format, and DLC).
             */
            std::byte get_type_byte() const;

            /**
             * @brief Set the type byte.
             */
            Result<void> set_type_byte(std::byte type_byte);

            // ==== UTILITIES ====

            /**
             * @brief Calculate the expected frame size for given parameters.
             */
            static std::size_t calculate_frame_size(bool extended_id, std::size_t data_length);

            /**
             * @brief Validate frame structure without checksum.
             */
            bool validate_structure() const;
    };
}