/**
 * @file config.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Configuration interface for the USB-CAN Bridge library.
 * @version 0.1
 * @date 2025-10-03
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "core.hpp"
namespace USBCANBridge {
    template<typename Frame>
    class ConfigInterface : public CoreInterface<Frame> {
        // * Ensure that Derived is ConfigFrame
        static_assert(is_config_frame_v<Frame>,
            "Derived must be a config frame type");

        protected:

            // * Prevent this class from being instantiated directly
            ConfigInterface() : CoreInterface<Frame>() {
                static_assert(!std::is_same_v<Frame, ConfigInterface>,
                    "ConfigInterface cannot be instantiated directly");
            }

        public:
            // === Configuration Methods ===

            /**
             * @brief Get the baud rate currently set in the configuration frame.
             * @return CANBaud The current baud rate
             */
            CANBaud get_baud_rate() const {
                return from_byte<CANBaud>(this->frame_storage_[this->layout_.BAUD]);
            }

            /**
             * @brief Set the CAN bus baud rate to be used by the USB adapter.
             * @param baud_rate The baud rate to set.
             */

            void set_baud_rate(CANBaud baud_rate) {
                this->frame_storage_[this->layout_.BAUD] = to_byte(baud_rate);
                // Mark checksum as dirty since we changed the frame
                this->derived().mark_dirty();
            }

            /**
             * @brief Set the CAN mode to be used by the USB adapter.
             * @param mode The CAN mode to set.
             */
            void set_can_mode(CANMode mode) {
                this->frame_storage_[this->layout_.MODE] = to_byte(mode);
                // Mark checksum as dirty since we changed the frame
                this->derived().mark_dirty();
            }

            /**
             * @brief Get the CAN mode currently set in the configuration frame.
             * @return CANMode The current CAN mode
             */

            CANMode get_can_mode() const {
                return from_byte<CANMode>(this->frame_storage_[this->layout_.MODE]);
            }

            /**
             * @brief Set the acceptance filter of CAN frames for the USB adapter.
             * The filter is an hexadecimal value that determines which CAN IDs are accepted.
             * @note The filter must be set to match the lower 11 bits for standard frames (`0x0..000` to `0x0..7FF`) or the lower 29 bits for extended frames (`0x00000000` to `0x1FFFFFFF`).
             * @param filter The acceptance filter to set.
             */
            void set_filter(uint32_t filter) {
                // Store the given filter in big-endian format
                for (size_t i = 0; i < this->layout_.FILTER_SIZE; ++i) {
                    this->frame_storage_[
                        this->layout_.FILTER + this->layout_.FILTER_SIZE - 1 - i
                    ] = (static_cast<std::uint8_t>(filter & 0xFF));
                    filter >>= 8;
                }
                // Mark checksum as dirty since we changed the frame
                this->derived().mark_dirty();
            }

            /**
             * @brief Get the acceptance filter currently set in the configuration frame.
             * The filter is an hexadecimal value that determines which CAN IDs are accepted.
             * @note The filter must be set to match the lower 11 bits for standard frames (`0x0..000` to `0x0..7FF`) or the lower 29 bits for extended frames (`0x00000000` to `0x1FFFFFFF`).
             * @return uint32_t The current acceptance filter
             */
            uint32_t get_filter() const {
                // Read the filter from big-endian format
                uint32_t filter = 0;
                for (size_t i = 0; i < this->layout_.FILTER_SIZE; ++i) {
                    filter = (filter << 8) | static_cast<uint8_t>(
                        this->frame_storage_[this->layout_.FILTER + i]
                    );
                }
                return filter;
            }

            /**
             * @brief Set the acceptance mask of CAN frames for the USB adapter.
             * The mask is an hexadecimal value that determines which bits of the CAN ID are relevant for filtering.
             * @note The mask must be set to match the lower 11 bits for standard frames (`0x0..000` to `0x0..7FF`) or the lower 29 bits for extended frames (`0x00000000` to `0x1FFFFFFF`).
             * @param mask The acceptance mask to set.
             */
            void set_mask(uint32_t mask) {
                // Store the given mask in big-endian format
                for (size_t i = 0; i < this->layout_.MASK_SIZE; ++i) {
                    this->frame_storage_[
                        this->layout_.MASK + this->layout_.MASK_SIZE - 1 - i
                    ] = (static_cast<std::uint8_t>(mask & 0xFF));
                    mask >>= 8;
                }
                // Mark checksum as dirty since we changed the frame
                this->derived().mark_dirty();
            }

            /**
             * @brief Get the acceptance mask currently set in the configuration frame.
             * The mask is an hexadecimal value that determines which bits of the CAN ID are relevant for filtering.
             * @note The mask must be set to match the lower 11 bits for standard frames (`0x0..000` to `0x0..7FF`) or the lower 29 bits for extended frames (`0x00000000` to `0x1FFFFFFF`).
             * @return uint32_t The current acceptance mask
             */
            uint32_t get_mask() const {
                // Read the mask from big-endian format
                uint32_t mask = 0;
                for (size_t i = 0; i < this->layout_.MASK_SIZE; ++i) {
                    mask = (mask << 8) | static_cast<uint8_t>(
                        this->frame_storage_[this->layout_.MASK + i]
                    );
                }
                return mask;
            }

            /**
             * @brief Enable or disable automatic retransmission of CAN frames.
             * @param auto_rtx RTX::ENABLED to enable, RTX::DISABLED to disable.
             */
            void set_auto_rtx(RTX auto_rtx) {
                this->frame_storage_[this->layout_.AUTO_RTX] = to_byte(auto_rtx);
                // Mark checksum as dirty since we changed the frame
                this->derived().mark_dirty();
            }

            /**
             * @brief Get the current automatic retransmission setting.
             * @return RTX RTX::ENABLED if enabled, RTX::DISABLED if disabled
             */
            RTX get_auto_rtx() const {
                return from_byte<RTX>(this->frame_storage_[this->layout_.AUTO_RTX]);
            }


    };
}
