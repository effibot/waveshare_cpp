/**
 * @file config_validator.hpp
 * @author Andrea Efficace (andrea.efficace1@gmail.com)
 * @brief Configuration validation utilities for frame types.
 * @version 0.1
 * @date 2025-10-02
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "../template/result.hpp"
#include "../template/frame_traits.hpp"
#include "../enums/protocol.hpp"

namespace USBCANBridge {
    /**
     * @brief Configuration validation utilities for frame types.
     * @tparam Derived The derived frame type.
     * @note This class uses CRTP to call derived class implementations for specific validations when needed.
     */
    template<typename Derived>
    class ConfigValidator {
        protected:
            // * CRTP helper to access derived class methods
            Derived& derived() {
                return static_cast<Derived&>(*this);
            }
            // * CRTP helper to access derived class methods (const version)
            const Derived& derived() const {
                return static_cast<const Derived&>(*this);
            }
        public:
            /**
             * @brief Validate the given baud rate against the supported rates.
             * @param rate The CAN baud rate to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @see CANBaud in protocol.hpp for supported rates.
             */
            Result<bool> validateBaudRate(CANBaud rate) const {
                switch (rate) {
                case CANBaud::BAUD_10K:
                case CANBaud::BAUD_20K:
                case CANBaud::BAUD_50K:
                case CANBaud::BAUD_100K:
                case CANBaud::BAUD_125K:
                case CANBaud::BAUD_250K:
                case CANBaud::BAUD_500K:
                case CANBaud::BAUD_800K:
                case CANBaud::BAUD_1M:
                    return Result<bool>::success(true);
                default:
                    return Result<bool>::error(Status::WBAD_CAN_BAUD);
                }
            }
            /**
             * @brief Validate the given CAN mode against the supported modes.
             * @param mode The CAN mode to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @see CANMode in protocol.hpp for supported modes.
             */
            Result<bool> validateCanMode(CANMode mode) const {
                switch (mode) {
                case CANMode::NORMAL:
                case CANMode::SILENT:
                case CANMode::LOOPBACK:
                case CANMode::LOOPBACK_SILENT:
                    return Result<bool>::success(true);
                default:
                    return Result<bool>::error(Status::WBAD_CAN_MODE);
                }
            }
            /**
             * @brief Validate the given acceptance filter value.
             * The filter mask has to match the selected CAN ID type (standard or extended).
             * @param filter The acceptance filter to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note The acceptance filter is a 32-bit mask. Any value inside the CAN2.0A ([`0x0`-`0x7FF`]) or CAN2.0B ([`0x0`-`0x1FFFFFFF`]) range is considered valid.
             */
            Result<bool> validateFilter(uint32_t filter) const {
                auto frame_type = this->derived().get_frame_type();
                if (!frame_type) {
                    return frame_type.error();
                }

                // Check frame type and corresponding filter limits
                switch (frame_type.value()) {
                case FrameType::STD_VARIABLE:
                case FrameType::STD_FIXED:
                    return filter <= MAX_CAN_ID_STD ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_FILTER);

                case FrameType::EXT_VARIABLE:
                case FrameType::EXT_FIXED:
                    return filter <= MAX_CAN_ID_EXT ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_FILTER);

                default:
                    return Result<bool>::error(Status::WBAD_FILTER);
                }
            }
            /**
             * @brief Validate the given acceptance mask value.
             * The mask has to match the selected CAN ID type (standard or extended).
             * @param mask The acceptance mask to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @note The acceptance mask is a 32-bit mask. Any value inside the CAN2.0A ([`0x0`-`0x7FF`]) or CAN2.0B ([`0x0`-`0x1FFFFFFF`]) range is considered valid.
             */
            Result<bool> validateMask(uint32_t mask) const {
                auto frame_type = this->derived().get_frame_type();
                if (!frame_type) {
                    return frame_type.error();
                }
                // Check frame type and corresponding mask limits
                switch (frame_type.value()) {
                case FrameType::STD_VARIABLE:
                case FrameType::STD_FIXED:
                    return mask <= MAX_CAN_ID_STD ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_MASK);
                case FrameType::EXT_VARIABLE:
                case FrameType::EXT_FIXED:
                    return mask <= MAX_CAN_ID_EXT ?
                           Result<bool>::success(true) :
                           Result<bool>::error(Status::WBAD_MASK);
                default:
                    return Result<bool>::error(Status::WBAD_MASK);
                }
            }

            /**
             * @brief Validate the given RTX (retransmission) setting.
             * @param rtx The RTX setting to validate.
             * @return Result<bool> True if valid, false if invalid, or an error status on failure.
             * @see RTX in protocol.hpp for supported settings.
             */
            Result<bool> validateRTX(RTX rtx) const {
                switch (rtx) {
                case RTX::AUTO:
                case RTX::OFF:
                    return Result<bool>::success(true);
                default:
                    return Result<bool>::error(Status::WBAD_RTX);
                }
            }


    };
}