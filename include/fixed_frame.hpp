#pragma once

#include "base_frame.hpp"
#include "span_compat.hpp"

namespace USBCANBridge {
    /**
     * @brief Fixed frame implementation.
     *
     * This class represents a fixed-size frame in the USBCANBridge library.
     * It inherits from BaseFrame and provides specific functionality for
     * 20-byte fixed frames with checksum validation and CAN data transport.
     *
     * Features:
     * - 20-byte fixed size with compile-time layout
     * - Automatic checksum calculation and validation
     * - CAN ID and data payload support
     * - Type-safe operations through BaseFrame interface
     */
    class FixedFrame : public BaseFrame<FixedFrame> {
        private:
            using Traits = frame_traits_t<FixedFrame>;
            using Layout = layout_t<FixedFrame>;

            alignas(4) mutable typename Traits::StorageType storage_;

            // Optimization: cache checksum calculation
            mutable bool checksum_dirty_ = true;
            mutable std::byte cached_checksum_ = std::byte{0};
            /**
             * @brief Initialize fixed fields of the frame.
             */
            void init_fixed_fields();

            /**
             * @brief Calculate and cache checksum if dirty.
             */
            void update_checksum() const;

        public:
            /**
             * @brief Default constructor.
             * Initializes a FixedFrame with default values and proper field setup.
             */
            FixedFrame();

            ~FixedFrame() = default;

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

            // Fixed frame specific operations
            Result<void> impl_set_frame_type(FrameType type);
            bool impl_verify_checksum() const;

            // ==== ADDITIONAL UTILITIES ====

            /**
             * @brief Check if frame uses extended CAN ID.
             */
            bool is_extended_id() const;

            /**
             * @brief Get frame type.
             */
            Result<FrameType> get_frame_type() const;

            // ==== CHECKSUM UTILITIES ====

            /**
             * @brief Calculate checksum for raw byte data (static utility).
             */
            static uint8_t calculate_checksum(const std::byte* data, std::size_t size);

            /**
             * @brief Validate checksum in raw byte data (static utility).
             */
            static bool validate_checksum(const std::byte* data, std::size_t size);

            /**
             * @brief Force checksum recalculation.
             */
            void mark_dirty() const {
                checksum_dirty_ = true;
            }
    };
}