#pragma once

#include "base_frame.hpp"
#include "frame_traits.hpp"
#include "span_compat.hpp"

namespace USBCANBridge {
    /**
     * @brief Configuration Frame implementation.
     *
     * This class represents a configuration frame in the USBCANBridge library.
     * It inherits from BaseFrame and provides specific functionality for
     * 20-byte configuration frames with baud rate, filter, and mode settings.
     *
     * Features:
     * - 20-byte fixed size with compile-time layout
     * - Automatic checksum calculation and validation
     * - Baud rate, filter, mask, and mode configuration
     * - Type-safe operations through BaseFrame interface
     */
    class ConfigFrame : public BaseFrame<ConfigFrame> {
        private:
            using Traits = frame_traits_t<ConfigFrame>;
            using Layout = layout_t<ConfigFrame>;

            alignas(8) mutable storage_t<ConfigFrame> storage_;

            // Optimization: cache checksum calculation
            mutable bool checksum_dirty_ = true;

            /**
             * @brief Initialize fixed fields of the frame.
             */
            void impl_init_fixed_fields();

            /**
             * @brief Calculate and cache checksum if dirty.
             */
            void update_checksum() const;

        public:
            /**
             * @brief Default constructor.
             * Initializes a ConfigFrame with default values and proper field setup.
             */
            ConfigFrame();

            ~ConfigFrame() = default;

            // %  BASEFRAME INTERFACE IMPLEMENTATIONS

            // Universal operations
            Result<void> impl_serialize(span<std::byte> buffer) const;
            Result<void> impl_deserialize(span<const std::byte> data);
            Result<bool> impl_validate() const;
            span<const std::byte> impl_get_raw_data() const;
            Result<std::size_t> impl_size() const;
            Result<void> impl_clear();

            // Config frame operations
            Result<void> impl_set_baud_rate(CANBaud rate);
            Result<CANBaud> impl_get_baud_rate() const;
            Result<void> impl_set_filter(uint32_t filter);
            Result<uint32_t> impl_get_filter() const;
            Result<void> impl_set_mask(uint32_t mask);
            Result<uint32_t> impl_get_mask() const;


            /**
             * @brief Set configuration mode.
             */
            Result<void> set_mode(CANMode mode);

            /**
             * @brief Get configuration mode.
             */
            Result<CANMode> get_mode() const;

            /**
             * @brief Verify checksum.
             */
            bool impl_verify_checksum() const;

    };
}