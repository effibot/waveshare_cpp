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
#include "config_validator.hpp"
namespace USBCANBridge {
    template<typename Frame>
    class ConfigInterface : public CoreInterface<Frame> {
        // * Ensure that Derived is ConfigFrame
        static_assert(is_config_frame_v<Frame>,
            "Derived must be a config frame type");
        protected:
            // * Get validator instance for config operations
            ConfigValidator<Frame> config_validator_;

            // * Prevent this class from being instantiated directly
            ConfigInterface() : CoreInterface<Frame>(), config_validator_(this->derived()) {
                static_assert(!std::is_same_v<Frame, ConfigInterface>,
                    "ConfigInterface cannot be instantiated directly");
                // Initialize constant fields
                this->derived().impl_init_fields();
            }


        public:
            // <<< Decorate serialize to use checksum_interface_ >>>
            /**
             * @brief Decoration of CoreInterface::serialize to use checksum_interface_.
             *
             * @param buffer
             * @return  Result<span<const std::byte> > A span representing the serialized frame data. The length of the span is fixed to FRAME_SIZE for fixed-size frames, or the current size for variable-size frames.
             */
            Result<span<const std::byte> > serialize(span<std::byte> buffer) const {
                // Update checksum before serialization
                auto checksum_res = this->derived().update_checksum();
                if (!checksum_res) {
                    return Result<void>::error(checksum_res.error(), "ConfigInterface::serialize");
                }
                // Call the base class serialize method
                return CoreInterface<Frame>::serialize(buffer);
            }

            // === Configuration Methods ===
            /**
             * @brief Set the CAN bus baud rate to be used by the USB adapter.
             * @param baud_rate The baud rate to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_baud_rate(CANBaud baud_rate) {
                auto valid = config_validator_.validateBaudRate(baud_rate);
                if (!valid) {
                    return Result<void>::error(Status::WBAD_CAN_BAUD,
                        "ConfigInterface::set_baud_rate");
                }
                return this->derived().impl_set_baud_rate(baud_rate);
            }
            /**
             * @brief Get the baud rate currently set in the configuration frame.
             * @return Result<CANBaud> The current baud rate, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<CANBaud> >
            get_baud_rate() const {
                return this->derived().impl_get_baud_rate();
            }
            /**
             * @brief Set the CAN mode to be used by the USB adapter.
             * @param mode The CAN mode to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_can_mode(CANMode mode) {
                auto valid = config_validator_.validateCanMode(mode);
                if (!valid) {
                    return Result<void>::error(Status::WBAD_CAN_MODE,
                        "ConfigInterface::set_can_mode");
                }
                return this->derived().impl_set_can_mode(mode);
            }
            /**
             * @brief Get the CAN mode currently set in the configuration frame.
             * @return Result<CANMode> The current CAN mode, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<CANMode> >
            get_can_mode() const {
                return this->derived().impl_get_can_mode();
            }
            /**
             * @brief Set the acceptance filter of CAN frames for the USB adapter.
             * @param filter The acceptance filter to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_filter(uint32_t filter) {
                auto valid = config_validator_.validateFilter(filter);
                if (!valid) {
                    return Result<void>::error(Status::WBAD_FILTER, "ConfigInterface::set_filter");
                }
                return this->derived().impl_set_filter(filter);
            }
            /**
             * @brief Get the acceptance filter currently set in the configuration frame.
             * @return Result<uint32_t> The current acceptance filter, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<uint32_t> >
            get_filter() const {
                return this->derived().impl_get_filter();
            }
            /**
             * @brief Set the acceptance mask of CAN frames for the USB adapter.
             * @param mask The acceptance mask to set.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_mask(uint32_t mask) {
                auto valid = config_validator_.validateMask(mask);
                if (!valid) {
                    return Result<void>::error(Status::WBAD_MASK, "ConfigInterface::set_mask");
                }
                return this->derived().impl_set_mask(mask);
            }
            /**
             * @brief Get the acceptance mask currently set in the configuration frame.
             * @return Result<uint32_t> The current acceptance mask, or an error status on failure.
             */
            template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<uint32_t> >
            get_mask() const {
                return this->derived().impl_get_mask();
            }
            /**
             * @brief Enable or disable automatic retransmission of CAN frames.
             * @param auto_rtx RTX::ENABLED to enable, RTX::DISABLED to disable.
             * @return Result<void> Status::SUCCESS on success, or an error status on failure.
             */template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<void> >
            set_auto_rtx(RTX auto_rtx) {
                auto valid = config_validator_.validateRtx(auto_rtx);
                if (!valid) {
                    return Result<void>::error(Status::WBAD_RTX, "ConfigInterface::set_auto_rtx");
                }
                return this->derived().impl_set_auto_rtx(auto_rtx);
            }
            /**
             * @brief Get the current automatic retransmission setting.
             * @return Result<RTX> RTX::ENABLED if enabled, RTX::DISABLED if disabled, or an error status on failure.
             */template<typename T = Frame>
            std::enable_if_t<is_config_frame_v<T>, Result<RTX> >
            get_auto_rtx() const {
                return this->derived().impl_get_auto_rtx();
            }


    };
}